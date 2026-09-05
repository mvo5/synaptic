/* fake_sources_dir.h - a throwaway apt tree for source-list tests
 *
 * Copyright (c) 2026 Synaptic development team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#pragma once

#include <apt-pkg/configuration.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <glib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>

// Creates <TMPDIR>/synaptic-test-XXXXXX with a sources.list.d/ inside and
// points apt's Dir::Etc::sourceparts and Dir::Etc::sourcelist at it, so the
// code under test sees exactly what the repository dialog would see.
class FakeSourcesDir
{
 public:
   std::string root;
   std::string parts;
   std::string main;

   FakeSourcesDir()
   {
      GError *err = nullptr;
      gchar *dir = g_dir_make_tmp("synaptic-test-XXXXXX", &err);
      if (dir == nullptr) {
         std::cerr << "FAIL: cannot create a temporary directory: "
                   << err->message << std::endl;
         std::exit(1);
      }
      root = dir;
      g_free(dir);
      parts = root + "/sources.list.d";
      main = root + "/sources.list";
      mkdir(parts.c_str(), 0755);
      _config->Set("Dir::Etc::sourceparts", parts);
      _config->Set("Dir::Etc::sourcelist", main);
   }

   ~FakeSourcesDir()
   {
      std::error_code ec;
      std::filesystem::remove_all(root, ec);
   }

   // Paths are relative to the fake tree's root, e.g.
   // "sources.list.d/a.sources".
   std::string path(const std::string &rel) const
   {
      return root + "/" + rel;
   }

   void put(const std::string &rel, const std::string &body) const
   {
      std::ofstream out(path(rel), std::ios::binary);
      out << body;
   }

   std::string get(const std::string &rel) const
   {
      std::ifstream in(path(rel), std::ios::binary);
      std::ostringstream ss;
      ss << in.rdbuf();
      return ss.str();
   }
};
