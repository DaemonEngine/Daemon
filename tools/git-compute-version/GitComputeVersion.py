#! /usr/bin/env python3

# Daemon BSD Source Code
# Copyright (c) 2024-2026, Daemon Developers
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
import os.path
import subprocess
import sys
import time

class GitComputeVersion():
	git_short_ref_length = 7

	def __init__(self, source_dir, allows_stray, is_quiet, is_local):
		if not os.path.isdir(source_dir):
			raise(ValueError, "not a directory")

		self.process_stderr = None

		if is_quiet:
			self.process_stderr = subprocess.DEVNULL

		self.is_local = is_local

		self.source_dir_realpath = os.path.realpath(source_dir)

		self.git_command_list = ["git", "-C", self.source_dir_realpath]

		# Test that Git is available and working.
		self.runGitCommand(["-v"])

		self.allows_stray = allows_stray

	def runGitCommand(self, command_list):
		command_list = self.git_command_list + command_list

		process = subprocess.run(command_list,
			stdout=subprocess.PIPE, stderr=self.process_stderr, text=True, check=True)

		return process.stdout.rstrip()

	def getDateString(self, timestamp):
		return datetime.datetime.fromtimestamp(timestamp, datetime.UTC).strftime('%Y%m%d-%H%M%S')

	def isGitDirty(self):
		if self.is_local:
			lookup_dir_arg = [self.source_dir_realpath]
		else:
			lookup_dir_arg = []

		git_status_porcelain_string = self.runGitCommand(
			["status", "--porcelain", "--"] + lookup_dir_arg)

		return git_status_porcelain_string != ""

	def getVersionString(self):
		# Fallback version string for stray repositories (not tracked by git yet).
		tag_string="0"
		date_string="-" + self.getDateString(time.time())
		ref_string=""
		dirt_string="+stray"

		# Git returns an error if the directory is not a Git repository or it has no
		# commits. It also may return an error if the repo is broken somehow. We fail
		# to distinguish these cases and assume it is not a Git repository.
		try:
			git_last_commit_string = self.runGitCommand(
				["rev-parse", "HEAD", "--"])
		except subprocess.CalledProcessError:
			if not self.allows_stray:
				raise
			return tag_string + date_string + ref_string + dirt_string

		# Now we know we must use a Git-based version string.
		dirt_string = ""

		# Git prints the current commit reference.
		git_last_commit_short_string = git_last_commit_string[:self.git_short_ref_length]
		ref_string = "-" + git_last_commit_short_string

		# Git prints the current commit date.
		git_last_commit_timestamp_string = self.runGitCommand(
			["log", "-1", "--pretty=format:%ct"])
		date_string = "-" + self.getDateString(int(git_last_commit_timestamp_string))

		# Git prints the most recent tag, or a commit hash if there is no tag at all.
		git_closest_tag_string = self.runGitCommand(
			["describe", "--always", "--tags", "--abbrev=0", "--match", "v[0-9]*"])

		# If a tag is found:
		if git_closest_tag_string[0] == "v":
			git_closest_tag_version_string = git_closest_tag_string[1:]
			tag_string = git_closest_tag_version_string

			# Git prints a version string that is equal to the most recent tag
			# if the most recent tag is on the current commit.
			git_describe_tag_string = self.runGitCommand(
				["describe", "--tags", "--match", "v[0-9]*"])
			git_describe_version_string = git_describe_tag_string[1:]

			if git_closest_tag_version_string == git_describe_version_string:
				# Do not write current commit reference and date in version
				# string if the tag is on the current commit.
				date_string = ""
				ref_string = ""

		if self.isGitDirty():
			# Write the dirty flag in version string if not everything in
			# the Git repository is properly committed.
			dirt_string = "+dirty"

		return tag_string + date_string + ref_string + dirt_string

def getVersionString(source_dir, allows_stray=False, is_quiet=False, is_local=False):
	return GitComputeVersion(source_dir, allows_stray, is_quiet, is_local).getVersionString()

def main():
	import argparse

	def existing_dir(path):
		if not os.path.isdir(path):
			raise argparse.ArgumentTypeError(f"{path} is not an existing directory")
		return path

	parser = argparse.ArgumentParser(description="Print repository version string")

	parser.add_argument("-s", "--stray", dest="allows_stray", help="allows stray repositories not tracked by Git", action="store_true")
	parser.add_argument("-q", "--quiet", dest="is_quiet", help="silence Git errors", action="store_true")
	parser.add_argument("-l", "--local", dest="is_local", help="look for dirtiness in given directory only, not in whole repository", action="store_true")
	parser.add_argument(dest="source_dir", nargs="?", metavar="DIRNAME", type=existing_dir, default=".", help="repository path")

	args = parser.parse_args()

	print(getVersionString(args.source_dir, allows_stray=args.allows_stray, is_quiet=args.is_quiet, is_local=args.is_local))

if __name__ == "__main__":
	main()
