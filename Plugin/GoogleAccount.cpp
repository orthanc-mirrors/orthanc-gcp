/**
 * Google Cloud Platform credentials for DICOMweb and Orthanc
 * Copyright (C) 2019-2020 Osimis S.A., Belgium
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


#include "GoogleAccount.h"

#include <Toolbox.h>

void GoogleAccount::LoadAuthorizedUser(const std::string& json)
{
  google::cloud::StatusOr<google::cloud::storage::oauth2::AuthorizedUserCredentialsInfo> info = 
    google::cloud::storage::oauth2::ParseAuthorizedUserCredentials(json, "memory");

  if (!info)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                    "Cannot parse authorized user configuration");
  }
  else
  {
    type_ = Type_AuthorizedUser;
    authorizedUser_.reset(new google::cloud::storage::oauth2::AuthorizedUserCredentialsInfo(*info));
  }
}


bool GoogleAccount::LoadServiceAccount(const OrthancPlugins::OrthancConfiguration& account)
{
  std::string path;

  if (!account.LookupStringValue(path, "ServiceAccountFile"))
  {
    return false;
  }

  OrthancPlugins::MemoryBuffer f;
  f.ReadFile(path);
    
  std::string s;
  f.ToString(s);
    
  google::cloud::StatusOr<google::cloud::storage::oauth2::ServiceAccountCredentialsInfo> info = 
    google::cloud::storage::oauth2::ParseServiceAccountCredentials(s, "memory");

  if (!info)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                    "Cannot parse service account configuration at: " + path);
  }

  type_ = Type_ServiceAccount;
  serviceAccount_.reset(new google::cloud::storage::oauth2::ServiceAccountCredentialsInfo(*info));
  return true;
}


bool GoogleAccount::LoadAuthorizedUserFile(const OrthancPlugins::OrthancConfiguration& account)
{
  std::string path;

  if (account.LookupStringValue(path, "AuthorizedUserFile"))
  {
    OrthancPlugins::MemoryBuffer f;
    f.ReadFile(path);

    std::string s;
    f.ToString(s);

    LoadAuthorizedUser(s);
    return true;
  }
  else
  {
    return false;
  }
}


bool GoogleAccount::LoadAuthorizedUserStrings(const OrthancPlugins::OrthancConfiguration& account)
{
  std::string clientId, clientSecret, refreshToken;

  if (account.LookupStringValue(clientId, "AuthorizedUserClientId") &&
      account.LookupStringValue(clientSecret, "AuthorizedUserClientSecret") &&
      account.LookupStringValue(refreshToken, "AuthorizedUserRefreshToken"))
  {
    Json::Value json = Json::objectValue;
    json["client_id"] = clientId;
    json["client_secret"] = clientSecret;
    json["refresh_token"] = refreshToken;

    LoadAuthorizedUser(json.toStyledString());
    return true;
  }
  else
  {
    return false;
  }
}


GoogleAccount::GoogleAccount(const OrthancPlugins::OrthancConfiguration& account,
                             const std::string& name) :
  name_(name)
{
  if (!account.LookupStringValue(project_, "Project"))
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                    "Missing \"Project\" option for account \"" + name + "\"");
  }

  if (!account.LookupStringValue(location_, "Location"))
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                    "Missing \"Location\" option for account \"" + name + "\"");
  }

  if (!account.LookupStringValue(dataset_, "Dataset"))
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                    "Missing \"Dataset\" option for account \"" + name + "\"");
  }

  if (!account.LookupStringValue(dicomStore_, "DicomStore"))
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                    "Missing \"DicomStore\" option for account \"" + name + "\"");
  }

  if (!LoadServiceAccount(account) &&
      !LoadAuthorizedUserFile(account) &&
      !LoadAuthorizedUserStrings(account))
  {
    throw Orthanc::OrthancException(
      Orthanc::ErrorCode_BadFileFormat,
      "Missing \"ServiceAccount\" or \"AuthorizedUserXXX\" option for account \"" + name + "\"");
  }
}


const google::cloud::storage::oauth2::AuthorizedUserCredentialsInfo& GoogleAccount::GetAuthorizedUser() const
{
  if (authorizedUser_.get() == NULL)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
  }
  else
  {
    return *authorizedUser_;
  }
}


google::cloud::storage::oauth2::ServiceAccountCredentialsInfo& GoogleAccount::GetServiceAccount() const
{
  if (serviceAccount_.get() == NULL)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
  }
  else
  {
    return *serviceAccount_;
  }
}


static std::string AddTrailingSlash(const std::string& url)
{
  // Add a trailing slash if needed
  if (url.empty() ||
      url[url.size() - 1] != '/')
  {
    return url + '/';
  }
  else
  {
    return url;
  }
}


bool GoogleAccount::UpdateServerDefinition(const std::string& dicomWebPluginRoot,
                                           const std::string& baseGoogleUrl,
                                           const std::string& token) const
{
  std::string url = (AddTrailingSlash(baseGoogleUrl) +
                     "projects/" + project_ + 
                     "/locations/" + location_ +
                     "/datasets/" + dataset_ +
                     "/dicomStores/" + dicomStore_ + 
                     "/dicomWeb/");

  size_t colon = token.find(':');
  if (colon == std::string::npos)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }

  std::string headerKey = Orthanc::Toolbox::StripSpaces(token.substr(0, colon));
  std::string headerValue = Orthanc::Toolbox::StripSpaces(token.substr(colon + 1));

  Json::Value headers = Json::objectValue;
  headers[headerKey] = headerValue;

  Json::Value server = Json::objectValue;
  server["Url"] = url;
  server["HasDelete"] = "1";   // Google Cloud Platform allows "-X DELETE"
  server["HttpHeaders"] = headers;

  Json::Value answer;
  if (OrthancPlugins::RestApiPut(answer, AddTrailingSlash(dicomWebPluginRoot) + "servers/" + name_, 
                                 server, true))
  {
    return true;
  }
  else
  {
    LOG(ERROR) << "Cannot update DICOMweb access to Google Cloud Platform: " << name_;
    return false;
  }
}
