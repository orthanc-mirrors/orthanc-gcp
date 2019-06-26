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



#include "GoogleConfiguration.h"


#define DEFAULT_GOOGLE_URL "https://healthcare.googleapis.com/v1beta1/"
#define DEFAULT_DICOMWEB_PLUGIN_ROOT "/dicom-web"


#include <boost/thread/mutex.hpp>


#define HAS_ORTHANC_FRAMEWORK_1_5_7  0    // TODO - Update to 1.5.7 once available + CMakeLists.txt
 

GoogleConfiguration::GoogleConfiguration()
{
  OrthancPlugins::OrthancConfiguration configuration;
  caInfo_ = configuration.GetStringValue("HttpsCACertificates", "");
  httpsVerifyPeers_ = configuration.GetBooleanValue("HttpsVerifyPeers", true);
    
  {
#if HAS_ORTHANC_FRAMEWORK_1_5_7 == 1
    OrthancPlugins::OrthancConfiguration dicomWeb(false);
#else
    OrthancPlugins::OrthancConfiguration dicomWeb;
#endif

    configuration.GetSection(dicomWeb, "DicomWeb");
    dicomWebPluginRoot_ = dicomWeb.GetStringValue("Root", DEFAULT_DICOMWEB_PLUGIN_ROOT);
  }

  {
#if HAS_ORTHANC_FRAMEWORK_1_5_7 == 1
    OrthancPlugins::OrthancConfiguration google(false);
#else
    OrthancPlugins::OrthancConfiguration google;
#endif

    configuration.GetSection(google, "GoogleCloudPlatform");

    baseGoogleUrl_ = google.GetStringValue("BaseUrl", DEFAULT_GOOGLE_URL);
    timeoutSeconds_ = google.GetUnsignedIntegerValue("Timeout", 10);
      
    if (!google.LookupUnsignedIntegerValue(refreshIntervalSeconds_, "RefreshInternal") ||
        refreshIntervalSeconds_ == 0)
    {
      refreshIntervalSeconds_ = 60;
    }

#if HAS_ORTHANC_FRAMEWORK_1_5_7 == 1
    OrthancPlugins::OrthancConfiguration accounts(false);
#else
    OrthancPlugins::OrthancConfiguration accounts;
#endif

    google.GetSection(accounts, "Accounts");

    const Json::Value::Members members = accounts.GetJson().getMemberNames();
    if (members.size() == 0)
    {
      LOG(WARNING) << "No Google Cloud Platform account is configured";
    }
    else
    {
      Reserve(members.size());

      for (size_t i = 0; i < members.size(); i++)
      {
        const std::string name = members[i];
        LOG(INFO) << "Adding Google Cloud Platform account: " << name;

#if HAS_ORTHANC_FRAMEWORK_1_5_7 == 1
        OrthancPlugins::OrthancConfiguration account(false);
#else
        OrthancPlugins::OrthancConfiguration account;
#endif

        accounts.GetSection(account, name);
        AddAccount(new GoogleAccount(account, name));
      }
    }
  }
}


GoogleConfiguration::~GoogleConfiguration()
{
  for (size_t i = 0; i < accounts_.size(); i++)
  {
    assert(accounts_[i] != NULL);
    delete accounts_[i];
  }
}


void GoogleConfiguration::AddAccount(GoogleAccount* account)   // Takes ownership
{
  if (account == NULL)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
  }
  else
  {
    accounts_.push_back(account);
  }
}


const GoogleAccount& GoogleConfiguration::GetAccount(size_t i) const
{
  if (i >= accounts_.size())
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
  }
  else
  {
    assert(accounts_[i] != NULL);
    return *accounts_[i];
  }
}


const GoogleConfiguration& GoogleConfiguration::GetInstance()
{
  static boost::mutex mutex_;
  static std::unique_ptr<GoogleConfiguration>  configuration_;

  {
    boost::mutex::scoped_lock lock(mutex_);

    if (configuration_.get() == NULL)
    {
      configuration_.reset(new GoogleConfiguration);
    }

    return *configuration_;
  }
}
