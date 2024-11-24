#! /usr/bin/env python3

import os
from mpmath import *

script_realpath = os.path.realpath(__file__)
root_dir = os.path.dirname(os.path.dirname(os.path.dirname(script_realpath)))
script_path = os.path.relpath(script_realpath, root_dir)
file_path = "src/common/math/Constant.h"
file_realpath = os.path.join(root_dir, file_path)

header = "// Generated with {}\n".format(script_path)
header += "// Do not modify!"

constants = []

class _phi:
	name = "phi"
	gnu = ""
	std = "phi"
	operation = "φ"
	def compute(): return mp.phi

constants.append(_phi)

class _egamma:
	name = "egamma"
	gnu = ""
	std = "egamma"
	operation = "Γe"
	def compute(): return gamma(e)

constants.append(_egamma)

class _ln2:
	name = "ln2"
	gnu = "M_LN2"
	std = "ln2"
	operation = "ln(2)"
	def compute(): return ln(2)

constants.append(_ln2)

class _ln10:
	name = "ln10"
	gnu = "M_LN10"
	std = "ln10"
	operation = "ln(10)"
	def compute(): return ln(10)

constants.append(_ln10)

class _log2e:
	name = "log2e"
	gnu = "M_LOG2E"
	std = "log2e"
	operation = "log2(e)"
	def compute(): return log(mp.e, 2)

constants.append(_log2e)

class _log10e:
	name = "log10e"
	gnu = "M_LOG10E"
	std = "log10e"
	operation = "log10(e)"
	def compute(): return log(mp.e, 10)

constants.append(_log10e)

class _pi:
	name = "pi"
	gnu = "M_PI"
	std = "pi"
	operation = "π"
	def compute(): return mp.pi

constants.append(_pi)

class _mul2_pi:
	name = "mul2_pi"
	gnu = ""
	std = ""
	operation = "2π"
	def compute(): return fmul(2, mp.pi)

constants.append(_mul2_pi)

class _inv_pi:
	name = "inv_pi"
	gnu = "M_1_PI"
	std = "inv_pi"
	operation = "1÷π"
	def compute(): return fdiv(1, mp.pi)

constants.append(_inv_pi)

class _inv_mul2_pi:
	name = "inv_mul2_pi"
	gnu = ""
	std = ""
	operation = "1÷2π"
	def compute(): return fdiv(1, fmul(2, mp.pi))

constants.append(_inv_mul2_pi)

class _div2_pi:
	name = "div2_pi"
	gnu = "M_2_PI"
	std = ""
	operation = "2÷π"
	def compute(): return fdiv(2, mp.pi)

constants.append(_div2_pi)

class _div180_pi:
	name = "div180_pi"
	gnu = ""
	std = ""
	operation = "180÷π"
	def compute(): return fdiv(180, mp.pi)

constants.append(_div180_pi)

class _divpi_2:
	name = "divpi_2"
	gnu = "M_PI_2"
	std = ""
	operation = "π÷2"
	def compute(): return fdiv(mp.pi, 2)

constants.append(_divpi_2)

class _divpi_4:
	name = "divpi_4"
	gnu = "M_PI_4"
	std = ""
	operation = "π÷4"
	def compute(): return fdiv(mp.pi, 4)

constants.append(_divpi_4)

class _divpi_90:
	name = "divpi_90"
	gnu = ""
	std = ""
	operation = "π÷90"
	def compute(): return fdiv(mp.pi, 90)

constants.append(_divpi_90)

class _divpi_180:
	name = "divpi_180"
	gnu = ""
	std = ""
	operation = "π÷180"
	def compute(): return fdiv(mp.pi, 180)

constants.append(_divpi_180)

class _divpi_360:
	name = "divpi_360"
	gnu = ""
	std = ""
	operation = "π÷360"
	def compute(): return fdiv(mp.pi, 360)

constants.append(_divpi_360)

class _sqrtpi:
	name = "sqrtpi"
	gnu = ""
	std = "sqrtpi"
	operation = "√π"
	def compute(): return sqrt(mp.pi)

constants.append(_sqrtpi)

class _sqrt2:
	name = "sqrt2"
	gnu = "M_SQRT2"
	std = "sqrt2"
	operation = "√2"
	def compute(): return sqrt(2)

constants.append(_sqrt2)

class _sqrt3:
	name = "sqrt3"
	gnu = ""
	std = "sqrt3"
	operation = "√3"
	def compute(): return sqrt(3)

constants.append(_sqrt3)

class _inv_sqrtpi:
	name = "inv_sqrtpi"
	gnu = ""
	std = "inv_sqrtpi"
	operation = "1÷√π"
	def compute(): return sqrt(mp.pi)

constants.append(_inv_sqrtpi)

class _inv_sqrt2:
	name = "inv_sqrt2"
	gnu = "M_SQRT1_2"
	std = "inv_sqrt2"
	operation = "1÷√2"
	def compute(): return fdiv(1, sqrt(2))

constants.append(_inv_sqrt2)

class _inv_sqrt3:
	name = "inv_sqrt3"
	gnu = ""
	std = "inv_sqrt3"
	operation = "1÷√3"
	def compute(): return fdiv(1, sqrt(3))

class _div2_sqrtpi:
	name = "div2_sqrtpi"
	gnu = "M_2_SQRTPI"
	std = ""
	operation = "2÷√π"
	def compute(): return fdiv(2, sqrt(mp.pi))

constants.append(_div2_sqrtpi)

constants.append(_inv_sqrt3)

types = []

class _float:
	name = "float"
	slug = "_f"
	suffix = "f"
	has_std = True
	std_suffix = "f"
	precision = 9

types.append(_float)

class _double:
	name = "double"
	slug = "_d"
	suffix = ""
	has_std = True
	std_suffix = ""
	precision = 20

types.append(_double)

class _long_double:
	name = "long double"
	slug = "_l"
	suffix = "l"
	has_std = False
	std_suffix = ""
	precision = 37

# types.append(_long_double)

keyword = "constexpr"

keyword_pad = len(keyword)

type_head = "Type"
name_head = "Name"
value_head = "Value"
operation_head = "Operation"
gnu_head = "GNU"
std_head = "std::numbers"

name_pad = len(name_head)
operation_pad = len(operation_head)
gnu_pad = len(gnu_head)

file_handle = open(file_realpath, "w")

def output(string=""):
	print(string)
	print(string, file=file_handle)

output(header)
output()
output("#ifndef COMMON_CONSTANT_H_")
output("#define COMMON_CONSTANT_H_")
output()
output("namespace Math {")

first = True

for t in types:
	if first:
		first = False
	else:
		output()

	mp.dps = t.precision

	type_pad = len(t.name)
	value_pad = len(str(_pi.compute()))
	suffix_pad = len(t.suffix)

	for c in constants:
		name = c.name + t.slug

		l = len(name)
		if l > name_pad:
			name_pad = l

		l = len(c.operation)
		if l > operation_pad:
			operation_pad = l

		gnu = c.gnu
		if gnu:
			gnu = gnu + t.suffix

		l = len(gnu)
		if l > gnu_pad:
			gnu_pad = l

		if t.has_std:
			std = std_head
		else:
			std = ""

	sizes = keyword_pad, type_pad, name_pad, value_pad, suffix_pad, operation_pad, gnu_pad
	string = "\t{:<%s} {:<%s} {:<%s} {} {:<%s}{:<%s}{} {} {:<%s}  {:<%s}  {}" % sizes

	fields = "//", type_head, name_head, " ", value_head, "", " ", "  ", operation_head, gnu_head, std
	output(string.format(*fields).rstrip())

	for c in constants:
		name = c.name + t.slug

		gnu = c.gnu
		if gnu:
			gnu = gnu + t.suffix

		if t.has_std:
			std = c.std
			if std:
				std = std + t.std_suffix
		else:
			std = ""

		v = str(c.compute())
		if v.startswith("0."):
			v = v[1:]

		v = v[:t.precision + 1]

		if "." in v:
			v = v.ljust(t.precision + 1, "0")

		fields = keyword, t.name, name, "=", v, t.suffix, ";", "//", c.operation, gnu, std
		output(string.format(*fields).rstrip())

output("}")
output()
output("#endif // COMMON_CONSTANT_H_")

file_handle.close()
