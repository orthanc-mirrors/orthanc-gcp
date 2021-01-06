/**
 * Google Cloud Platform credentials for DICOMweb and Orthanc
 * Copyright (C) 2019-2021 Osimis S.A., Belgium
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


#pragma once

#include "../Resources/Orthanc/Plugins/OrthancPluginCppWrapper.h"

#include <google/cloud/storage/oauth2/authorized_user_credentials.h>
#include <google/cloud/storage/oauth2/service_account_credentials.h>


class GoogleAccount : public boost::noncopyable
{
public:
  enum Type
  {
    Type_AuthorizedUser,
    Type_ServiceAccount
  };

private:
  Type         type_;
  std::string  name_;
  std::string  project_;
  std::string  location_;
  std::string  dataset_;
  std::string  dicomStore_;

  std::unique_ptr<google::cloud::storage::oauth2::AuthorizedUserCredentialsInfo>  authorizedUser_;
  std::unique_ptr<google::cloud::storage::oauth2::ServiceAccountCredentialsInfo>  serviceAccount_;


  void LoadAuthorizedUser(const std::string& json);

  bool LoadServiceAccount(const OrthancPlugins::OrthancConfiguration& account);

  bool LoadAuthorizedUserFile(const OrthancPlugins::OrthancConfiguration& account);

  bool LoadAuthorizedUserStrings(const OrthancPlugins::OrthancConfiguration& account);

public:
  GoogleAccount(const OrthancPlugins::OrthancConfiguration& account,
                const std::string& name);

  Type GetType() const
  {
    return type_;
  }

  const std::string& GetName() const
  {
    return name_;
  }

  const std::string& GetProject() const
  {
    return project_;
  }

  const std::string& GetLocation() const
  {
    return location_;
  }

  const std::string& GetDataset() const
  {
    return dataset_;
  }

  const std::string& GetDicomStore() const
  {
    return dicomStore_;
  }

  const google::cloud::storage::oauth2::AuthorizedUserCredentialsInfo& GetAuthorizedUser() const;

  google::cloud::storage::oauth2::ServiceAccountCredentialsInfo& GetServiceAccount() const;

  bool UpdateServerDefinition(const std::string& dicomWebPluginRoot,
                              const std::string& baseGoogleUrl,
                              const std::string& token) const;
};
