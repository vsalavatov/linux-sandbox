#!/bin/env python
import unittest
import os
from subprocess import Popen, PIPE

sandbox_executable = "./build/sandbox/sandbox"
common_options = ''


class TestSandbox(unittest.TestCase):
    @staticmethod
    def get_sandbox_output(options, executable, args):
        cmd = f'{sandbox_executable} {common_options} {options} -- {executable} {args}'
        with Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE) as proc:
            output, stderr = proc.communicate()
            return output.decode("utf-8"), stderr.decode("utf-8")

    @staticmethod
    def run_sandbox(options, executable, args):
        cmd = f'{sandbox_executable} {common_options} {options} -- {executable} {args}'
        return Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)


    def test_simple(self):
        executable = './build/examples/echo42/echo42'
        output, _ = self.get_sandbox_output('', executable, '')
        self.assertEqual('42', output.strip())

    def test_memory(self):
        executable = './build/examples/eat100mb/eat100mb'

        output1, _ = self.get_sandbox_output('-m 50000000', executable, '')
        self.assertEqual('gonna eat >= 104857600 bytes', output1.strip())

        output2, _ = self.get_sandbox_output('', executable, '')
        self.assertEqual('gonna eat >= 104857600 bytes\nyummy!', output2.strip())

    def test_forks(self):
        executable = './build/examples/forks/forks'

        output, _ = self.get_sandbox_output('-f 50', executable, '')
        self.assertEqual('Failed to fork (i = 48): Resource temporarily unavailable', output.strip())

    def test_file_access(self):
        os.mkdir('test_data')
        os.mkdir('test_data/mounted')
        os.system('echo outside > test_data/file_outside')
        os.system('echo inside > test_data/mounted/file_inside')

        try:
            output, stderr = self.get_sandbox_output('', '/bin/cat', 'test_data/file_outside')
            self.assertEqual('outside\n', output)

            output, stderr = self.get_sandbox_output('', '/bin/cat', '/mnt/file_inside')
            self.assertIn('/bin/cat: /mnt/file_inside: No such file or directory', stderr.split('\n'))

            
            output, stderr = self.get_sandbox_output('-r -i rootfs -a test_data/mounted:/mnt', '/bin/cat', 'test_data/file_outside')
            self.assertIn('/bin/cat: test_data/file_outside: No such file or directory', stderr.split('\n'))

            output, stderr = self.get_sandbox_output('-r -i rootfs -a test_data/mounted:/mnt', '/bin/cat', '/mnt/file_inside')
            self.assertEqual('inside\n', output)
        finally: 
            os.system('rm -rf test_data')

    def test_time(self):
        executable = './build/examples/sleep30/sleep30'
        output, stderr = self.get_sandbox_output('-t 1', executable, '')
        self.assertIn('(Sandbox) process has exceeded its time limit', stderr)

    def test_daemon(self):
        executable = './build/examples/daemon/daemon'
        output, stderr = self.get_sandbox_output('', executable, '')
        self.assertEqual('0. Parent pid: 2\n1. Child pid: 3\n2. Child pid: 4\nDaemon started.\nDaemon terminated.', output.strip())

    def test_killparent(self):
        executable = './build/examples/killparent/killparent'
        output, stderr = self.get_sandbox_output('', executable, '')
        self.assertEqual('Failed to kill watcher.', output.strip())

    def test_niceness(self):
        executable = './build/examples/sleep30/sleep30'
        for expected_ni in [-7, 11]:
            with self.run_sandbox(f'-n {expected_ni} -t 1', executable, ''):
                with os.popen("sh -c 'ps -p $(pidof sleep30) -o ni='") as proc:
                    niceness = int(proc.read())
                self.assertEqual(niceness, expected_ni)

    
if __name__ == '__main__':
    current_directory = os.getcwd()
    unittest.main()
