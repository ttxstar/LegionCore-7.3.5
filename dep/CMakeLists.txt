# Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

add_subdirectory(threads)
add_subdirectory(boost)
add_subdirectory(zlib)
add_subdirectory(g3dlite)
add_subdirectory(recastnavigation)
add_subdirectory(fmt)
add_subdirectory(SFMT)
add_subdirectory(utf8cpp)
add_subdirectory(openssl)
add_subdirectory(mysql)
add_subdirectory(readline)
add_subdirectory(gsoap)
add_subdirectory(rapidjson)
add_subdirectory(protobuf)
add_subdirectory(efsw)

if(TOOLS)
  add_subdirectory(bzip2)
  add_subdirectory(CascLib)
endif()
