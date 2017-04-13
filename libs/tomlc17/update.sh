#! /usr/bin/env bash

cd "$(dirname "${BASH_SOURCE[0]}")"

# tomlcpp.hpp requires C++20.
for file in tomlc17.c tomlc17.h # tomlcpp.hpp
do
	wget -O "${file}" "https://github.com/cktan/tomlc17/raw/refs/heads/main/src/${file}"
done
