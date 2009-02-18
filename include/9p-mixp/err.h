/* (C)opyright MMIV-MMVI Anselm R. Garbe <garbeam at gmail dot com>
 * Copyright ©2006-2007 Kris Maglione <fbsdaemon@gmail.com>
 * See LICENSE file for license details.
 */

#ifndef __MIXP_ERR_H
#define __MIXP_ERR_H

const char *mixp_errbuf();
void  mixp_errstr(char*, int);
void  mixp_rerrstr(char*, int);
void  mixp_werrstr(char*, ...);

#endif
