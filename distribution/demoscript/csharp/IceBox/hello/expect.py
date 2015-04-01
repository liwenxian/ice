#!/usr/bin/env python
# **********************************************************************
#
# Copyright (c) 2003-2015 ZeroC, Inc. All rights reserved.
#
# This copy of Ice is licensed to you under the terms described in the
# ICE_LICENSE file included in this distribution.
#
# **********************************************************************

import sys, os

path = [ ".", "..", "../..", "../../..", "../../../.." ]
head = os.path.dirname(sys.argv[0])
if len(head) > 0:
    path = [os.path.join(head, p) for p in path]
path = [os.path.abspath(p) for p in path if os.path.exists(os.path.join(p, "demoscript")) ]
if len(path) == 0:
    raise RuntimeError("can't find toplevel directory!")
sys.path.append(path[0])

from demoscript import Util
from demoscript.IceBox import hello

if Util.defaultHost:
    args = ' --IceBox.Service.Hello="helloservice.dll:HelloServiceI --Ice.Config=config.service %s"' % Util.defaultHost
else:
    args = ''

server = Util.spawn("%s --Ice.Config=config.icebox --Ice.PrintAdapterReady %s" % (Util.getIceBox("csharp"), args))
server.expect('.* ready')
client = Util.spawn('client.exe')
client.expect('.*==>')

hello.run(client, server)