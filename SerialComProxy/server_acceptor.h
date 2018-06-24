// client_acceptor.h,v 1.5 1999/09/22 03:13:38 jcej Exp

#ifndef CLIENT_ACCEPTOR_H
#define CLIENT_ACCEPTOR_H

#include "ace/Acceptor.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/SOCK_Acceptor.h"

#include "CmdHandler_T.h"
typedef ACE_Acceptor <CCmdHandler_T, ACE_SOCK_ACCEPTOR> Cmd_Acceptor;

#endif /* CLIENT_ACCEPTOR_H */
