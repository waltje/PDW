/*
 *		Define application version and build info.
 *
 * Version:	@(#)version.h	1.0.1	2024/03/23
 *
 * Author:	Fred N. van Kempen, <waltje@varcem.com>
 *
 *		Copyright 2024 Fred N. van Kempen.
 *
 *		Redistribution and  use  in source  and binary forms, with
 *		or  without modification, are permitted  provided that the
 *		following conditions are met:
 *
 *		1. Redistributions of  source  code must retain the entire
 *		   above notice, this list of conditions and the following
 *		   disclaimer.
 *
 *		2. Redistributions in binary form must reproduce the above
 *		   copyright  notice,  this list  of  conditions  and  the
 *		   following disclaimer in  the documentation and/or other
 *		   materials provided with the distribution.
 *
 *		3. Neither the  name of the copyright holder nor the names
 *		   of  its  contributors may be used to endorse or promote
 *		   products  derived from  this  software without specific
 *		   prior written permission.
 *
 * THIS SOFTWARE  IS  PROVIDED BY THE  COPYRIGHT  HOLDERS AND CONTRIBUTORS
 * "AS IS" AND  ANY EXPRESS  OR  IMPLIED  WARRANTIES,  INCLUDING, BUT  NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE  ARE  DISCLAIMED. IN  NO  EVENT  SHALL THE COPYRIGHT
 * HOLDER OR  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES;  LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON  ANY
 * THEORY OF  LIABILITY, WHETHER IN  CONTRACT, STRICT  LIABILITY, OR  TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING  IN ANY  WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VERSION_H
# define VERSION_H


/* Application name. */
#define APP_NAME	"PDW"
#define APP_TITLE	"Windows POCSAG, FLEX, ACARS, MOBITEX & ERMES Decoder"
#define APP_PRODUCT	"PDW"
#define APP_COMPANY	"The VARCem Group, LLC"
#define APP_COPYRIGHT	"Copyright 2001-2024 All Contributors."
#define APP_LICENSE	"Released under mixed GPL v3 and BSD 3-Clause License."


/* Version info. */
#define APP_VER_MAJOR	3
#define APP_VER_MINOR	2
#define APP_VER_REV	0
#define APP_VER_PATCH	2


/* Standard C preprocessor macros. */
#define STR_STRING(x)	#x
#define STR(x)		STR_STRING(x)
#define STR_RC(a,e)	a ## , ## e


/* These are used in the application. */
#define APP_VER_NUM	APP_VER_MAJOR.APP_VER_MINOR.APP_VER_REV
#if defined(APP_VER_PATCH) && APP_VER_PATCH > 0
# define APP_VER_NUM_4	APP_VER_MAJOR.APP_VER_MINOR.APP_VER_REV.APP_VER_PATCH
# define APP_VERSION	STR(APP_VER_NUM_4)
#else
# define APP_VER_NUM_4	APP_VER_MAJOR.APP_VER_MINOR.APP_VER_REV.0
# define APP_VERSION	STR(APP_VER_NUM)
#endif
#define APP_VERSION_4	STR(APP_VER_NUM_4)


#endif	/*VERSION_H*/
