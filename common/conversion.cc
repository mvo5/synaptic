/* -*- mode: cpp; mode: fold -*-
 *
 * conversion.cc
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
 * $Id: conversion.cc,v 1.1 2003/01/23 13:13:50 mvogt Exp $
 */

#include "conversion.h"

#include <langinfo.h>

#include <glib/gmem.h>
#include <glib/gconvert.h>

using namespace std;

IConv::IConv(const char *tocode, const char *fromcode):
    /*inbuf(0), inbuflen(0),*/
    outbuf(0), outbuflen(0), gbuf(0)
{
    //type = UseIConv;
    type = UseGConv;
    if (fromcode == NULL)
	fromcode = nl_langinfo(CODESET);
    if (tocode!=NULL && fromcode!=NULL)
	cd = iconv_open(tocode, fromcode);
    else
	cd = (iconv_t)-1;
}

IConv::~IConv()
{
    iconv_close(cd);
    delete outbuf;
    g_free(gbuf);
}

const char *IConv::i_convert(const char *frombuf, size_t len)
{
    if (frombuf==NULL || len<=0)
	return NULL;
    if (cd == (iconv_t)-1)
	return frombuf;

    if (outbuflen < 3*len+1) {
	delete outbuf;
	outbuf = new char[3*len+1];
	if (outbuf!=NULL)
	    outbuflen = 3*len+1;
	else outbuflen = 0;
    }
    char *inptr = (char *) frombuf;
    char *outptr = outbuf;
    size_t inleft = len;
    size_t outleft = outbuflen - 1;

    size_t res = iconv(cd, &inptr, &inleft, &outptr, &outleft);
    if (res != (size_t) -1) {
	*outptr = 0;
	return outbuf;
    } else
	return frombuf;
}

const char *IConv::g_convert(const char *frombuf, size_t len)
{
  gsize br = 0;
  gsize bw = 0;

  g_free(gbuf);

  gbuf = g_locale_to_utf8(frombuf, len, &br, &bw, NULL);

  return gbuf;
}
