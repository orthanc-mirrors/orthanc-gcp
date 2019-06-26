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



#pragma once

#include "GoogleAccount.h"

class GoogleConfiguration : public boost::noncopyable
{
private:
  std::string                  caInfo_;
  std::string                  baseGoogleUrl_;
  std::string                  dicomWebPluginRoot_;
  std::vector<GoogleAccount*>  accounts_;
  unsigned int                 timeoutSeconds_;
  unsigned int                 refreshIntervalSeconds_;
  bool                         httpsVerifyPeers_;

  GoogleConfiguration();  // Singleton pattern

public:
  ~GoogleConfiguration();

  void Reserve(size_t i)
  {
    accounts_.reserve(i);
  }

  void AddAccount(GoogleAccount* account);   // Takes ownership

  size_t GetAccountsCount() const
  {
    return accounts_.size();
  }

  const GoogleAccount& GetAccount(size_t i) const;

  const std::string& GetBaseGoogleUrl() const
  {
    return baseGoogleUrl_;
  }

  const std::string& GetDicomWebPluginRoot() const
  {
    return dicomWebPluginRoot_;
  }

  unsigned int GetRefreshIntervalSeconds() const
  {
    return refreshIntervalSeconds_;
  }

  const std::string& GetCaInfo() const
  {
    return caInfo_;
  }

  unsigned int GetTimeoutSeconds() const
  {
    return timeoutSeconds_;
  }

  bool IsHttpsVerifyPeers() const
  {
    return httpsVerifyPeers_;
  }

  static const GoogleConfiguration& GetInstance();
};
