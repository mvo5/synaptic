'''apport package hook for synaptic

(c) 2011 Canonical Ltd.
Author: Brian Murray <brian@ubuntu.com>
'''

from apport.hookutils import *


def add_info(report):
    attach_file_if_exists(report, '/var/log/apt/history.log',
        'DpkgHistoryLog.txt')
    attach_root_command_outputs(report,
        {'DpkgTerminalLog.txt': 'cat /var/log/apt/term.log',
         'CurrentDmesg.txt': 'dmesg | comm -13 --nocheck-order /var/log/dmesg -'})
