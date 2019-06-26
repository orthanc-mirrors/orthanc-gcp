/**
 * Google Cloud Platform credentials for DICOMweb and Orthanc
 * Copyright (C) 2019 Osimis S.A., Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "GoogleUpdater.h"

#include "GoogleConfiguration.h"

#include <google/cloud/storage/internal/curl_handle_factory.h>
#include <google/cloud/storage/oauth2/google_credentials.h>


namespace
{
  class CurlBuilder : public google::cloud::storage::internal::CurlRequestBuilder
  {
  private:
    class HandleFactory : public google::cloud::storage::internal::DefaultCurlHandleFactory
    {
    public:
      google::cloud::storage::internal::CurlPtr CreateHandle() override
      {
        google::cloud::storage::internal::CurlPtr handle
          (google::cloud::storage::internal::DefaultCurlHandleFactory::CreateHandle());

        const GoogleConfiguration& configuration = GoogleConfiguration::GetInstance();

        long timeout = static_cast<long>(configuration.GetTimeoutSeconds());

        if (!configuration.GetCaInfo().empty() &&
            curl_easy_setopt(handle.get(), CURLOPT_CAINFO, configuration.GetCaInfo().c_str()) != CURLE_OK)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                          "Cannot set the trusted Certificate Authorities");
        }

        bool ok;
        
        if (configuration.IsHttpsVerifyPeers())
        {
          ok = (curl_easy_setopt(handle.get(), CURLOPT_SSL_VERIFYHOST, 2) == CURLE_OK &&
                curl_easy_setopt(handle.get(), CURLOPT_SSL_VERIFYPEER, 1) == CURLE_OK &&
                curl_easy_setopt(handle.get(), CURLOPT_TIMEOUT, timeout) == CURLE_OK);
        }
        else
        {
          ok = (curl_easy_setopt(handle.get(), CURLOPT_SSL_VERIFYHOST, 0) == CURLE_OK &&
                curl_easy_setopt(handle.get(), CURLOPT_SSL_VERIFYPEER, 0) == CURLE_OK);
        }

        if (!ok)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                          "Cannot initialize a libcurl handle");
        }

        return handle;
      }

      google::cloud::storage::internal::CurlMulti CreateMultiHandle() override
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }
    };

  public:
    CurlBuilder(std::string base_url,
                std::shared_ptr<google::cloud::storage::internal::CurlHandleFactory> factory) :
      CurlRequestBuilder(base_url, std::make_shared<HandleFactory>())
    {
    }
  };
}



static boost::posix_time::ptime GetNow()
{
  return boost::posix_time::second_clock::local_time();
} 


void GoogleUpdater::Worker(const State* state,
                           const GoogleAccount* account,
                           long refreshIntervalSeconds)
{
  std::shared_ptr<google::cloud::storage::oauth2::Credentials> credentials;

  try
  {
    switch (account->GetType())
    {
      case GoogleAccount::Type_ServiceAccount:
        credentials = std::make_shared<google::cloud::storage::oauth2::ServiceAccountCredentials
                                       <CurlBuilder>>(account->GetServiceAccount());
        break;

      case GoogleAccount::Type_AuthorizedUser:
        credentials = std::make_shared<google::cloud::storage::oauth2::AuthorizedUserCredentials
                                       <CurlBuilder>>(account->GetAuthorizedUser());
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }
  }
  catch (Orthanc::OrthancException& e)
  {
    credentials.reset();
  }

  if (credentials.get() == NULL)
  {
    LOG(ERROR) << "Cannot initialize the token updater for Google Cloud Platform account: " 
               << account->GetName();
    return;
  }

  const std::string dicomWebPluginRoot = GoogleConfiguration::GetInstance().GetDicomWebPluginRoot();
  const std::string baseGoogleUrl = GoogleConfiguration::GetInstance().GetBaseGoogleUrl();

  std::string lastToken;
  std::unique_ptr<boost::posix_time::ptime> lastUpdate;

  while (*state == State_Running)
  {
    if (lastUpdate.get() == NULL ||
        (GetNow() - *lastUpdate).total_seconds() >= refreshIntervalSeconds)
    {
      google::cloud::StatusOr<std::string> token = credentials->AuthorizationHeader();
      if (!token)
      {
        LOG(WARNING) << "Cannot generate Google Cloud Platform token for account: " << account->GetName();
      }
      else if (*token != lastToken &&
               account->UpdateServerDefinition(dicomWebPluginRoot, baseGoogleUrl, *token))
      {
        lastToken = *token;
      }

      lastUpdate.reset(new boost::posix_time::ptime(GetNow()));
    }
      
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
  }
}


GoogleUpdater::~GoogleUpdater()
{
  if (state_ == State_Running)
  {
    LOG(ERROR) << "GoogleUpdater::Stop() should have been manually called";
    Stop();
  }
}


void GoogleUpdater::Start()
{
  if (state_ != State_Setup)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
  }

  state_ = State_Running;

  const GoogleConfiguration& configuration = GoogleConfiguration::GetInstance();

  workers_.resize(configuration.GetAccountsCount());

  for (size_t i = 0; i < workers_.size(); i++)
  {
    workers_[i] = new boost::thread(Worker, &state_, &configuration.GetAccount(i), 
                                    configuration.GetRefreshIntervalSeconds());
  }
}

  
void GoogleUpdater::Stop()
{
  if (state_ == State_Running)
  {
    state_ = State_Done;

    for (size_t i = 0; i < workers_.size(); i++)
    {
      if (workers_[i] != NULL)
      {
        if (workers_[i]->joinable())
        {
          workers_[i]->join();
        }

        delete workers_[i];
      }
    }

    workers_.clear();
  }
}
