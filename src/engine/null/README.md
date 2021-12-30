This directory contains no-op implementations of functionality which may not be included in a given
binary. For example, the TTY client does not have sound, so it uses NullAudio.cpp. It is a kind of
link-time polymorphism - a given function prototype may have a real or a no-op implementation,
depending on which definition is linked in.
