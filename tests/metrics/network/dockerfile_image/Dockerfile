#  This file is part of cc-oci-runtime.
#
#  Copyright (C) 2017 Intel Corporation
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

# Usage: FROM [image name]
FROM ubuntu

# Ensure packages are current, then install:
#
# - a basic development environment
# - and additional general tooling
#
RUN apt-get update && apt-get install -y \
    build-essential \
    curl \
    smem \
    iperf3 \
    iperf

# Install nuttcp (Network performance measurement tool)
RUN cd $HOME && \
    curl -OkL "http://nuttcp.net/nuttcp/beta/nuttcp-7.3.2.c" && \
    gcc nuttcp-7.3.2.c -o nuttcp

CMD ["/bin/bash"]
