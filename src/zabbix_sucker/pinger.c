/* 
** Zabbix
** Copyright (C) 2000,2001,2002,2003 Alexei Vladishev
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/wait.h>

#include <string.h>

#ifdef HAVE_NETDB_H
	#include <netdb.h>
#endif

/* Required for getpwuid */
#include <pwd.h>

#include <signal.h>
#include <errno.h>

#include <time.h>

#include "common.h"
#include "cfg.h"
#include "db.h"
#include "functions.h"
#include "log.h"

#include "pinger.h"

int create_host_file(void)
{
	char	sql[MAX_STRING_LEN+1];
	FILE	*f;
	int	now;

	DB_HOST	host;
	DB_RESULT	*result;

	f = fopen("/tmp/zabbix_agentd.pinger", "w");

	if( f == NULL)
	{
		zabbix_log( LOG_LEVEL_CRIT, "Cannot hosts file [%s] [%s]",
		"/tmp/zabbix_agentd.pinger", strerror(errno));
		uninit();
		exit(-1);
	}

	now=time(NULL);
	sprintf(sql,"select h.useip,h.ip,h.host from hosts h where (h.status=%d or (h.status=%d and h.disable_until<=%d))", HOST_STATUS_MONITORED, HOST_STATUS_UNREACHABLE, now);
	result = DBselect(sql);
		
	for(i=0;i<DBnum_rows(result);i++)
	{
		host.useip=atoi(DBget_field(result,i,0));
		host.ip=DBget_field(result,i,1);
		host.host=DBget_field(result,i,2);

		if(HOST_USE_IP == host.useip)
		{
			fprintf(f,"%d",host.ip);
		}
		else
		{
			fprintf(f,"%d",host.host);
		}
	}
	DBfree_result(result);

	fclose(f);
}

int pinger_loop(void)
{
	for(;;)
	{
#ifdef HAVE_FUNCTION_SETPROCTITLE
		setproctitle("connecting to the database");
#endif
		DBconnect(CONFIG_DBHOST, CONFIG_DBNAME, CONFIG_DBUSER, CONFIG_DBPASSWORD, CONFIG_DBSOCKET);

		now=time(NULL);

		create_host_file();

/*		do_ping();

		update_items();*/

		DBclose();
#ifdef HAVE_FUNCTION_SETPROCTITLE
		setproctitle("pinger [sleeping for %d seconds]", CONFIG_PINGER_FREQUENCY);
#endif
		sleep(CONFIG_PINGER_FREQUENCY);
	}
}
