workspace "Fox"
	configurations { "Debug", "Release" }

project "Fox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"

	files {
		"*.h",
		"*.cpp",
		"utfcpp/**.h"
	}

	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"
