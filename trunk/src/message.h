/* Describes message structure used for communication
 * same as apps/messenger/messenger-event.hh
 */

#ifndef MESSAGE_H
#define MESSAGE_H 1

enum hoolock_cmd {MAKE_REQ, MAKE_ACK, BREAK_REQ, BREAK_ACK};

struct Hoolock_msg
{
	/* Message identifier.*/
	char msg_id[7];

	/* MAC address of host. */
	char hostMac[12];

	/* Separator. */
	char separator1;

	/* Hoolock command */
	hoolock_cmd cmd;
};


#endif
