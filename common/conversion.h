/* -*- mode: cpp; mode: fold -*-
 *
 * conversion.h
 * 
 * Copyright (c) 2003 Sviatoslav Sviridov <svd@altlinux.ru>
 * 
 * Author: Sviatoslav Sviridov <svd@altlinux.ru>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 * $Id: conversion.h,v 1.1 2003/01/23 13:13:50 mvogt Exp $
 */

#ifndef CONVERSION_H
#define CONVERSION_H

#include <stddef.h>
#include "config.h"
#include <iconv.h>
#include <glib/gconvert.h>

class IConv
{
    enum ConvType {UseIConv, UseGConv};

  protected:
    ConvType type;
    iconv_t cd;
 
    char *outbuf;
    size_t outbuflen;
 
    gchar *gbuf;
  public:
    IConv(const char *tocode, const char *fromcode = NULL);
    ~IConv();

    void i_reset() { iconv(cd, NULL, NULL, NULL, NULL); };
    const char *i_convert(const char *frombuf, size_t len);
    const char *g_convert(const char *frombuf, size_t len);

    const char *convert(const char *frombuf, size_t len)
      { return type == UseIConv ? i_convert(frombuf, len) : g_convert(frombuf, len); };

};

#endif // CONVERSION_H
