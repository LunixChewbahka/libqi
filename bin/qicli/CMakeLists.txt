##
## Author(s):
##  - Laurent LEC        <llec@aldebaran-robotics.com>
##
## Copyright (C) 2012 Aldebaran Robotics
##

project (QiCLI)

set(SRCS
  qicli.hpp
  sessionhelper.hpp
  servicehelper.hpp
  sessionhelper.cpp
  servicehelper.cpp
  almemoryhelper.hpp
  almemoryhelper.cpp
  main.cpp
  subcmds.cpp
  utils.cpp
  qitrace.cpp
  qitop.cpp
  qilog.cpp
  qimod.cpp
)

qi_create_bin(qicli
  ${SRCS}
  DEPENDS QI BOOST_PROGRAM_OPTIONS)

include_directories(./)
