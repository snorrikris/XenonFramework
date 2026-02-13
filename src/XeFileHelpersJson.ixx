module;

#include <fstream>
#include "logging.h"
#include <string>
#include <functional>
#include "nlohmann/json.hpp"

export module Xe.FileHelpersJson;

import Xe.StringTools;
import Xe.StringTools;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using json = nlohmann::json;

VSRL::Logger& logger_filehelperjson() { return VSRL::s_pVSRL->GetInstance("FileHelpersJson"); }

export bool LoadJsonDataFromFile(const std::wstring& pathname, std::function<void(json& j)> put_json_data_func)
{
	std::string error_string;
	try
	{
		json j;
		std::ifstream inpFile(pathname);
		if (inpFile.fail())
		{
			return false;
		}
		inpFile >> j;
		put_json_data_func(j);
	}
	catch (json::exception& e)
	{
		error_string = "LoadJsonDataFromFile failed for " + xet::to_astr(pathname)
			+ " - json parse error: " + e.what();
	}
	catch (const std::exception& ex)
	{
		error_string = "LoadJsonDataFromFile failed for " + xet::to_astr(pathname)
			+ " - json parse error: " + ex.what();
	}
	catch (...)
	{
		error_string = "LoadJsonDataFromFile failed for " + xet::to_astr(pathname)
			+ " - json parse error: unknown error";
	}

	if (error_string.size())
	{
		logger_filehelperjson().error(error_string.c_str());
		return false;
	}
	return true;
}


export bool SaveJsonDataToFile(const std::wstring& pathname, std::function<void(json& j)> get_json_data_func)
{
	std::string error_string;
	try
	{
		json j;
		get_json_data_func(j);
		std::ofstream o(pathname);
		o << std::setw(4) << j << std::endl;
	}
	catch (json::exception& e)
	{
		error_string = "SaveJsonDataToFile failed for " + xet::to_astr(pathname)
			+ " - json serialize error: " + e.what();
	}
	catch (const std::exception& ex)
	{
		error_string = "SaveJsonDataToFile failed for " + xet::to_astr(pathname)
			+ " - json serialize error: " + ex.what();
	}
	catch (...)
	{
		error_string = "SaveJsonDataToFile failed for " + xet::to_astr(pathname)
			+ " - json serialize error: unknown error";
	}
	if (error_string.size())
	{
		logger_filehelperjson().error(error_string.c_str());
		return false;
	}
	return true;
}

