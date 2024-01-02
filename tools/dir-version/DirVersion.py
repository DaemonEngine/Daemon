# Daemon BSD Source Code
# Copyright (c) 2024, Daemon Developers
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of the <organization> nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import datetime
import os
import subprocess
import sys
import time

class _DirVersion():
	git_short_ref_length = 7
	is_permissive = False

	def __init__(self, source_dir, is_permissive, is_quiet, is_whole):
		if not os.path.isdir(source_dir):
			raise(ValueError, "not a directory")

		self.process_stderr = None

		if is_quiet:
			self.process_stderr = subprocess.DEVNULL

		self.is_whole = is_whole

		self.source_dir_realpath = os.path.realpath(source_dir)

		self.git_command_list = ["git", "-C", self.source_dir_realpath]

		# Test that Git is available and working.
		self.runGitCommand(["-v"])

		self.is_permissive = is_permissive

	def runGitCommand(self, command_list, is_permissive=False):
		command_list = self.git_command_list + command_list

		process_check = not (self.is_permissive or is_permissive)

		process = subprocess.run(command_list,
			stdout=subprocess.PIPE, stderr=self.process_stderr, check=process_check, text=True)

		return process.stdout.rstrip(), process.returncode

	def getDateString(self, timestamp):
		return datetime.datetime.utcfromtimestamp(timestamp).strftime('%Y%m%d-%H%M%S')

	def isDirtyGit(self):
		if self.is_whole:
			# Git prints the Git repository root directory.
			git_show_toplevel_string, git_show_toplevel_returncode = \
				self.runGitCommand(["rev-parse", "--show-toplevel"])

			lookup_dir = git_show_toplevel_string.splitlines()[0]
		else:
			lookup_dir = self.source_dir_realpath

		# Git returns 1 if there is at least one modified file in the given directory.
		git_diff_quiet_string, git_diff_quiet_returncode \
			= self.runGitCommand(["diff", "--quiet", lookup_dir], is_permissive=True)

		if git_diff_quiet_returncode != 0:
			return True

		# Git prints the list of untracked files in the given directory.
		git_ls_untracked_string, git_ls_untracked_returncode \
			= self.runGitCommand(["ls-files", "-z", "--others", "--exclude-standard", lookup_dir])

		untracked_file_list = git_ls_untracked_string.split('\0')[:-1]

		return len(untracked_file_list) > 0

	def getVersionString(self):
		# Fallback version string.
		tag_string="0"
		date_string="-" + self.getDateString(time.time())
		ref_string=""
		dirt_string="+dirty"

		# Git returns 1 if the directory is not a Git repository.
		git_last_commit_string, git_last_commit_returncode \
			= self.runGitCommand(["rev-parse", "HEAD", "--"])

		# Git-based version string.
		if git_last_commit_returncode == 0:
			# Git prints the current commit reference.
			git_last_commit_short_string = git_last_commit_string[:self.git_short_ref_length]
			ref_string = "-" + git_last_commit_short_string

			# Git prints the current commit date.
			git_last_commit_timestamp_string, git_last_commit_timestamp_returncode \
				= self.runGitCommand(["log", "-1", "--pretty=format:%ct"])

			if git_last_commit_timestamp_returncode == 0:
				date_string = "-" + self.getDateString(int(git_last_commit_timestamp_string))

			# Git prints the most recent tag or returns 1 if there is not tag at all.
			git_closest_tag_string, git_closest_tag_returncode \
				= self.runGitCommand(["describe", "--tags", "--abbrev=0", "--match", "v[0-9].*"])

			if git_closest_tag_returncode == 0:
				git_closest_tag_version_string = git_closest_tag_string[1:]
				tag_string = git_closest_tag_version_string

				# Git prints a version string that is equal to the most recent tag
				# if the most recent tag is on the current commit or returns 1 if
				# there is no tag at all.
				git_describe_tag_string, git_describe_tag_returncode \
					= self.runGitCommand(["describe", "--tags", "--match", "v[0-9].*"])
				git_describe_version_string = git_describe_tag_string[1:]

				if git_describe_tag_returncode == 0:
					if git_closest_tag_version_string == git_describe_version_string:
						# Do not write current commit reference and date in version
						# string if the tag is on the current commit.
						date_string = ""
						ref_string = ""

			if not self.isDirtyGit():
				# Do not write the dirty flag in version string if everything in
				# the Git repository is properly committed.
				dirt_string = ""

		return tag_string + date_string + ref_string + dirt_string

def getVersionString(source_dir, is_permissive=False, is_quiet=False, is_whole=False):
	return _DirVersion(source_dir, is_permissive, is_quiet, is_whole).getVersionString()
