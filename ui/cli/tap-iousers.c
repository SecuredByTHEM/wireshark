/* tap-iousers.c
 * iostat   2003 Ronnie Sahlberg
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <epan/packet.h>
#include <epan/timestamp.h>
#include <ui/cli/tshark-tap.h>

typedef struct _io_users_t {
	const char *type;
	const char *filter;
	conv_hash_t hash;
} io_users_t;

static void
iousers_draw_human_readable(void *arg)
{
	conv_hash_t *hash = (conv_hash_t*)arg;
	io_users_t *iu = (io_users_t *)hash->user_data;
	conv_item_t *iui;
	guint64 last_frames, max_frames;
	struct tm * tm_time;
	guint i;
	gboolean display_ports = (!strncmp(iu->type, "TCP", 3) || !strncmp(iu->type, "UDP", 3) || !strncmp(iu->type, "SCTP", 4)) ? TRUE : FALSE;

	printf("================================================================================\n");
	printf("%s Conversations\n", iu->type);
	printf("Filter:%s\n", iu->filter ? iu->filter : "<No Filter>");

	switch (timestamp_get_type()) {
	case TS_ABSOLUTE:
	case TS_UTC:
		printf("%s                                               |       <-      | |       ->      | |     Total     | Absolute Time  |   Duration   |\n",
			display_ports ? "            " : "");
		printf("%s                                               | Frames  Bytes | | Frames  Bytes | | Frames  Bytes |      Start     |              |\n",
			display_ports ? "            " : "");
		break;
	case TS_ABSOLUTE_WITH_YMD:
	case TS_ABSOLUTE_WITH_YDOY:
	case TS_UTC_WITH_YMD:
	case TS_UTC_WITH_YDOY:
		printf("%s                                               |       <-      | |       ->      | |     Total     | Absolute Date  |   Duration   |\n",
			display_ports ? "            " : "");
		printf("%s                                               | Frames  Bytes | | Frames  Bytes | | Frames  Bytes |     Start      |              |\n",
			display_ports ? "            " : "");
		break;
	case TS_RELATIVE:
	case TS_NOT_SET:
	default:
		printf("%s                                               |       <-      | |       ->      | |     Total     |    Relative    |   Duration   |\n",
			display_ports ? "            " : "");
		printf("%s                                               | Frames  Bytes | | Frames  Bytes | | Frames  Bytes |      Start     |              |\n",
			display_ports ? "            " : "");
		break;
	}

	max_frames = UINT_MAX;
	do {
		last_frames = 0;
		for (i=0; (iu->hash.conv_array && i < iu->hash.conv_array->len); i++) {
			guint64 tot_frames;

			iui = &g_array_index(iu->hash.conv_array, conv_item_t, i);
			tot_frames = iui->rx_frames + iui->tx_frames;

			if ((tot_frames > last_frames) && (tot_frames < max_frames)) {
				last_frames = tot_frames;
			}
		}

		for (i=0; (iu->hash.conv_array && i < iu->hash.conv_array->len); i++) {
			guint64 tot_frames;
			char *src_addr, *dst_addr;

			iui = &g_array_index(iu->hash.conv_array, conv_item_t, i);
			tot_frames = iui->rx_frames + iui->tx_frames;

			if (tot_frames == last_frames) {
				/* XXX - TODO: make name / port resolution configurable (through gbl_resolv_flags?) */
				src_addr = get_conversation_address(NULL, &iui->src_address, TRUE);
				dst_addr = get_conversation_address(NULL, &iui->dst_address, TRUE);
				if (display_ports) {
					char *src, *dst, *src_port, *dst_port;
					src_port = get_conversation_port(NULL, iui->src_port, iui->ptype, TRUE);
					dst_port = get_conversation_port(NULL, iui->dst_port, iui->ptype, TRUE);
					src = wmem_strconcat(NULL, src_addr, ":", src_port, NULL);
					dst = wmem_strconcat(NULL, dst_addr, ":", dst_port, NULL);
					printf("%-26s <-> %-26s  %6" G_GINT64_MODIFIER "u %9" G_GINT64_MODIFIER
					       "u  %6" G_GINT64_MODIFIER "u %9" G_GINT64_MODIFIER "u  %6"
					       G_GINT64_MODIFIER "u %9" G_GINT64_MODIFIER "u  ",
						src, dst,
						iui->rx_frames, iui->rx_bytes,
						iui->tx_frames, iui->tx_bytes,
						iui->tx_frames+iui->rx_frames,
						iui->tx_bytes+iui->rx_bytes
					);
					wmem_free(NULL, src_port);
					wmem_free(NULL, dst_port);
					wmem_free(NULL, src);
					wmem_free(NULL, dst);
				} else {
					printf("%-20s <-> %-20s  %6" G_GINT64_MODIFIER "u %9" G_GINT64_MODIFIER
					       "u  %6" G_GINT64_MODIFIER "u %9" G_GINT64_MODIFIER "u  %6"
					       G_GINT64_MODIFIER "u %9" G_GINT64_MODIFIER "u  ",
						src_addr, dst_addr,
						iui->rx_frames, iui->rx_bytes,
						iui->tx_frames, iui->tx_bytes,
						iui->tx_frames+iui->rx_frames,
						iui->tx_bytes+iui->rx_bytes
					);
				}

				wmem_free(NULL, src_addr);
				wmem_free(NULL, dst_addr);

				switch (timestamp_get_type()) {
				case TS_ABSOLUTE:
					tm_time = localtime(&iui->start_abs_time.secs);
					if (tm_time != NULL) {
						printf("%02d:%02d:%02d",
							 tm_time->tm_hour,
							 tm_time->tm_min,
							 tm_time->tm_sec);
					} else
						printf("XX:XX:XX");
					break;
				case TS_ABSOLUTE_WITH_YMD:
					tm_time = localtime(&iui->start_abs_time.secs);
					if (tm_time != NULL) {
						printf("%04d-%02d-%02d %02d:%02d:%02d",
							 tm_time->tm_year + 1900,
							 tm_time->tm_mon + 1,
							 tm_time->tm_mday,
							 tm_time->tm_hour,
							 tm_time->tm_min,
							 tm_time->tm_sec);
					} else
						printf("XXXX-XX-XX XX:XX:XX");
					break;
				case TS_ABSOLUTE_WITH_YDOY:
					tm_time = localtime(&iui->start_abs_time.secs);
					if (tm_time != NULL) {
						printf("%04d/%03d %02d:%02d:%02d",
							 tm_time->tm_year + 1900,
							 tm_time->tm_yday + 1,
							 tm_time->tm_hour,
							 tm_time->tm_min,
							 tm_time->tm_sec);
					} else
						printf("XXXX/XXX XX:XX:XX");
					break;
				case TS_UTC:
					tm_time = gmtime(&iui->start_abs_time.secs);
					if (tm_time != NULL) {
						printf("%02d:%02d:%02d",
							 tm_time->tm_hour,
							 tm_time->tm_min,
							 tm_time->tm_sec);
					} else
						printf("XX:XX:XX");
					break;
				case TS_UTC_WITH_YMD:
					tm_time = gmtime(&iui->start_abs_time.secs);
					if (tm_time != NULL) {
						printf("%04d-%02d-%02d %02d:%02d:%02d",
							 tm_time->tm_year + 1900,
							 tm_time->tm_mon + 1,
							 tm_time->tm_mday,
							 tm_time->tm_hour,
							 tm_time->tm_min,
							 tm_time->tm_sec);
					} else
						printf("XXXX-XX-XX XX:XX:XX");
					break;
				case TS_UTC_WITH_YDOY:
					tm_time = gmtime(&iui->start_abs_time.secs);
					if (tm_time != NULL) {
						printf("%04d/%03d %02d:%02d:%02d",
							 tm_time->tm_year + 1900,
							 tm_time->tm_yday + 1,
							 tm_time->tm_hour,
							 tm_time->tm_min,
							 tm_time->tm_sec);
					} else
						printf("XXXX/XXX XX:XX:XX");
					break;
				case TS_RELATIVE:
				case TS_NOT_SET:
				default:
					printf("%14.9f",
						nstime_to_sec(&iui->start_time));
					break;
				}
				printf("   %12.4f\n",
					 nstime_to_sec(&iui->stop_time) - nstime_to_sec(&iui->start_time));
			}
		}
		max_frames = last_frames;
	} while (last_frames);
	printf("================================================================================\n");
}

static void
iousers_draw_machine_readable(void *arg)
{
	/* Prints out the conversations in a format that is easy for machine parsing */
	conv_hash_t *hash = (conv_hash_t*)arg;
	io_users_t *iu = (io_users_t *)hash->user_data;
	conv_item_t *iui;
	guint64 last_frames, max_frames;
	struct tm * tm_time;
	guint i;
	gboolean display_ports = (!strncmp(iu->type, "TCP", 3) || !strncmp(iu->type, "UDP", 3) || !strncmp(iu->type, "SCTP", 4)) ? TRUE : FALSE;;
	char null_string[] = "\0";

	max_frames = UINT_MAX;
	do {
		last_frames = 0;
		for (i=0; (iu->hash.conv_array && i < iu->hash.conv_array->len); i++) {
			guint64 tot_frames;

			iui = &g_array_index(iu->hash.conv_array, conv_item_t, i);
			tot_frames = iui->rx_frames + iui->tx_frames;

			if ((tot_frames > last_frames) && (tot_frames < max_frames)) {
				last_frames = tot_frames;
			}
		}

		for (i=0; (iu->hash.conv_array && i < iu->hash.conv_array->len); i++) {
			guint64 tot_frames;
			char *src_addr, *dst_addr, *src_port, *dst_port, *src_name, *dst_name;

			/**
			 * In specific cases, we won't have port information, so we'll simply leave
			 * these preset to a null string so they'll come out blank when we build the
			 * printf line
			 **/

			src_port = null_string;
			dst_port = null_string;

			iui = &g_array_index(iu->hash.conv_array, conv_item_t, i);
			tot_frames = iui->rx_frames + iui->tx_frames;

			if (tot_frames == last_frames) {
				/* XXX - TODO: make name / port resolution configurable (through gbl_resolv_flags?) */
				src_name = get_conversation_address(NULL, &iui->src_address, TRUE);
				dst_name = get_conversation_address(NULL, &iui->dst_address, TRUE);
				src_addr = get_conversation_address(NULL, &iui->src_address, FALSE);
				dst_addr = get_conversation_address(NULL, &iui->dst_address, FALSE);

				if (display_ports) {
					/* For CSV data, we'll simply render ports as integers instead of name */
					src_port = get_conversation_port(NULL, iui->src_port, iui->ptype, FALSE);
					dst_port = get_conversation_port(NULL, iui->dst_port, iui->ptype, FALSE);
				}

				/* If the src_name and src_addr match, it means that DNS resolution failed, zero it out */
				if (strcmp(src_name, src_addr) == 0) {
					src_name = null_string;
				}

				if (strcmp(dst_name, dst_addr) == 0) {
					dst_name = null_string;
				}

				printf("%s,%s,%s,%s,%s,%s,%s,%" G_GINT64_MODIFIER "u,%" G_GINT64_MODIFIER
						"u,%" G_GINT64_MODIFIER "u,%" G_GINT64_MODIFIER "u,%"
						G_GINT64_MODIFIER "u,%" G_GINT64_MODIFIER "u,",
					iu->type,
					src_addr, src_name, src_port,
					dst_addr, dst_name, dst_port,
					iui->rx_frames, iui->rx_bytes,
					iui->tx_frames, iui->tx_bytes,
					iui->tx_frames+iui->rx_frames,
					iui->tx_bytes+iui->rx_bytes
				);

				if (display_ports) {
					wmem_free(NULL, src_port);
					wmem_free(NULL, dst_port);
				}

				wmem_free(NULL, src_addr);
				wmem_free(NULL, dst_addr);


				tm_time = gmtime(&iui->start_abs_time.secs);
				if (tm_time != NULL) {
					printf("%04d-%02d-%02d %02d:%02d:%02d,",
							tm_time->tm_year + 1900,
							tm_time->tm_mon + 1,
							tm_time->tm_mday,
							tm_time->tm_hour,
							tm_time->tm_min,
							tm_time->tm_sec);
				} else {
					printf("XXXX-XX-XX XX:XX:XX,");
				}
				printf("%f\n",
					nstime_to_sec(&iui->stop_time) - nstime_to_sec(&iui->start_time));

			}
		}
		max_frames = last_frames;
	} while (last_frames);
}
static void
iousers_draw(void *arg)
{
	if (1) {
		iousers_draw_machine_readable(arg);
	} else {
		iousers_draw_human_readable(arg);
	}
}

void init_iousers(struct register_ct *ct, const char *filter)
{
	io_users_t *iu;
	GString *error_string;

	iu = g_new0(io_users_t, 1);
	iu->type = proto_get_protocol_short_name(find_protocol_by_id(get_conversation_proto_id(ct)));
	iu->filter = g_strdup(filter);
	iu->hash.user_data = iu;

	error_string = register_tap_listener(proto_get_protocol_filter_name(get_conversation_proto_id(ct)), &iu->hash, filter, 0, NULL, get_conversation_packet_func(ct), iousers_draw);
	if (error_string) {
		g_free(iu);
		fprintf(stderr, "tshark: Couldn't register conversations tap: %s\n",
		    error_string->str);
		g_string_free(error_string, TRUE);
		exit(1);
	}

}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
