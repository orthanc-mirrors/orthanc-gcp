/**
 * Google Cloud Platform credentials for DICOMweb and Orthanc
 * Copyright (C) 2019-2023 Osimis S.A., Belgium
 * Copyright (C) 2024-2025 Orthanc Team SRL, Belgium
 * Copyright (C) 2021-2025 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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
#include "GoogleUpdater.h"

#include "../Resources/Orthanc/Plugins/OrthancPluginCppWrapper.h"

#include <HttpClient.h>
#include <Logging.h>
#include <Toolbox.h>


static bool CheckDicomWebVersion()
{
  Json::Value dicomweb;
  if (!OrthancPlugins::RestApiGet(dicomweb, "/plugins/dicom-web", false))
  {
    LOG(ERROR) << "The DICOMweb plugin is not installed, cannot start Google Cloud Platform plugin";
    return false;
  }
  else if (dicomweb.type() != Json::objectValue ||
           !dicomweb.isMember("Version") ||
           dicomweb["Version"].type() != Json::stringValue)
  {
    LOG(ERROR) << "Cannot determine the version of the DICOMweb plugin, start Google Cloud Platform plugin";
    return false;
  }
  else
  {
    std::string version = dicomweb["Version"].asString();

    std::vector<std::string> tokens;
    Orthanc::Toolbox::TokenizeString(tokens, version, '.');
        
    bool ok = true;

    if (tokens.size() == 2)
    {
      try
      {
        int major = boost::lexical_cast<int>(tokens[0]);
        int minor = boost::lexical_cast<int>(tokens[1]);

        ok = (major >= 2 ||
              (major == 1 && minor >= 0));
      }
      catch (boost::bad_lexical_cast&)
      {
        ok = false;
      }
    }

    if (!ok)
    {
      LOG(ERROR) << "The DICOMweb version (currently " << version
                 << ") must be above 1.0 to use the Google Cloud Platform plugin";
    }

    return ok;
  }
}


OrthancPluginErrorCode OnChangeCallback(OrthancPluginChangeType changeType,
                                        OrthancPluginResourceType resourceType,
                                        const char* resourceId)
{
  try
  {
    switch (changeType)
    {
      case OrthancPluginChangeType_OrthancStarted:
      {
        if (CheckDicomWebVersion())
        {
          GoogleUpdater::GetInstance().Start();
        }

        break;
      }

      case OrthancPluginChangeType_OrthancStopped:
        GoogleUpdater::GetInstance().Stop();
        break;

      default:
        break;
    }
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "Exception in the change callback: " << e.What();
    return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
  }

  return OrthancPluginErrorCode_Success;
}



/**
 * We force the redefinition of the "ORTHANC_PLUGINS_API" macro, that
 * was left empty with gcc until Orthanc SDK 1.5.7 (no "default"
 * visibility). This causes the version script, if run from "Holy
 * Build Box", to make private the 4 global functions of the plugin.
 **/

#undef ORTHANC_PLUGINS_API

#ifdef WIN32
#  define ORTHANC_PLUGINS_API __declspec(dllexport)
#elif __GNUC__ >= 4
#  define ORTHANC_PLUGINS_API __attribute__((visibility ("default")))
#else
#  define ORTHANC_PLUGINS_API
#endif


extern "C"
{
  ORTHANC_PLUGINS_API int32_t OrthancPluginInitialize(OrthancPluginContext* context)
  {
    OrthancPlugins::SetGlobalContext(context);

#if ORTHANC_FRAMEWORK_VERSION_IS_ABOVE(1, 7, 2)
    Orthanc::Logging::InitializePluginContext(context);
#else
    Orthanc::Logging::Initialize(context);
#endif

    /* Check the version of the Orthanc core */
    if (OrthancPluginCheckVersion(context) == 0)
    {
      OrthancPlugins::ReportMinimalOrthancVersion(ORTHANC_PLUGINS_MINIMAL_MAJOR_NUMBER,
                                                  ORTHANC_PLUGINS_MINIMAL_MINOR_NUMBER,
                                                  ORTHANC_PLUGINS_MINIMAL_REVISION_NUMBER);
      return -1;
    }

    try
    {
      Orthanc::Toolbox::InitializeOpenSsl();
      Orthanc::HttpClient::GlobalInitialize();

      GoogleConfiguration::GetInstance();  // Force the initialization of the singleton

      OrthancPluginRegisterOnChangeCallback(context, OnChangeCallback);
    }
    catch (Orthanc::OrthancException& e)
    {
      OrthancPlugins::LogError("Error while initializing the Google Cloud Platform plugin: " + 
                               std::string(e.What()));
      return -1;
    }
  
    return 0;
  }


  ORTHANC_PLUGINS_API void OrthancPluginFinalize()
  {
    try
    {
      GoogleUpdater::GetInstance().Stop();
      Orthanc::HttpClient::GlobalFinalize();
      Orthanc::Toolbox::FinalizeOpenSsl();
    }
    catch (Orthanc::OrthancException&)
    {
    }
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetName()
  {
    return "google-cloud-platform";
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetVersion()
  {
    return GCP_PLUGIN_VERSION;
  }
}
