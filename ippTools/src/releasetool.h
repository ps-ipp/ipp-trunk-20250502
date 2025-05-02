/*
 * releasetool.h
 *
 * Copyright (C) 2013 University of Hawaii IFA 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * program; see the file COPYING. If not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef RELEASETOOL_H
#define RELEASETOOL_H 1

#include "pxtools.h"

typedef enum {
    RELEASETOOL_MODE_NONE      = 0x0,
    RELEASETOOL_MODE_DEFINESURVEY,
    RELEASETOOL_MODE_LISTSURVEY,
    RELEASETOOL_MODE_DEFINERELEASE,
    RELEASETOOL_MODE_UPDATERELEASE,
    RELEASETOOL_MODE_LISTRELEASE,
    RELEASETOOL_MODE_DEFINERELEXP,
    RELEASETOOL_MODE_UPDATERELEXP,
    RELEASETOOL_MODE_TOCALIBEXP,
    RELEASETOOL_MODE_LISTRELEXP,
    RELEASETOOL_MODE_DELETERELEXP,
    RELEASETOOL_MODE_DEFINERELSTACK,
    RELEASETOOL_MODE_UPDATERELSTACK,
    RELEASETOOL_MODE_SETRELSTACKCALIBRATEDFROMSKYCAL,
    RELEASETOOL_MODE_DELETERELSTACK,
    RELEASETOOL_MODE_TOCALIBSTACK,
    RELEASETOOL_MODE_LISTRELSTACK,
    RELEASETOOL_MODE_SUMMARY,
    RELEASETOOL_MODE_DEFINERELGROUP,
    RELEASETOOL_MODE_PENDINGRELGROUP,
    RELEASETOOL_MODE_UPDATERELGROUP,
    RELEASETOOL_MODE_LISTRELGROUP,
} releasetoolMode;

pxConfig *releasetoolConfig(pxConfig *config, int argc, char **argv);

#endif // RELEASETOOL_H
