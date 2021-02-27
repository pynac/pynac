"""
Pynac interface
"""

#*****************************************************************************
#       Copyright (C) 2008 William Stein <wstein@gmail.com>
#       Copyright (C) 2008 Burcin Erocal <burcin@erocal.org>
#       Copyright (C) 2017 Jeroen Demeyer <jdemeyer@cage.ugent.be>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#                  http://www.gnu.org/licenses/
#*****************************************************************************

from cpython cimport *
from ginac.pynac cimport *

cdef int GINAC_FN_SERIAL = 0

def set_ginac_fn_serial():
    """
    Initialize the GINAC_FN_SERIAL variable to the number of functions
    defined by GiNaC. This allows us to prevent collisions with C++ level
    special functions when a user asks to construct a symbolic function
    with the same name.
    """
    global GINAC_FN_SERIAL
    GINAC_FN_SERIAL = g_registered_functions().size()

cdef int py_get_ginac_serial():
    """
    Returns the number of C++ level functions defined by GiNaC.

    EXAMPLES::

        sage: from sage.libs.pynac.pynac import get_ginac_serial
        sage: get_ginac_serial() >= 35
        True
    """
    global GINAC_FN_SERIAL
    return GINAC_FN_SERIAL

def get_ginac_serial():
    """
    Number of C++ level functions defined by GiNaC. (Defined mainly for testing.)

    EXAMPLES::

        sage: sage.libs.pynac.pynac.get_ginac_serial() >= 35
        True
    """
    return py_get_ginac_serial()

cdef get_fn_serial_c():
    """
    Return overall size of Pynac function registry.
    """
    return g_registered_functions().size()

def get_fn_serial():
    """
    Return the overall size of the Pynac function registry which
    corresponds to the last serial value plus one.

    EXAMPLES::

        sage: from sage.libs.pynac.pynac import get_fn_serial
        sage: from sage.symbolic.function import get_sfunction_from_serial
        sage: get_fn_serial() > 125
        True
        sage: print(get_sfunction_from_serial(get_fn_serial()))
        None
        sage: get_sfunction_from_serial(get_fn_serial() - 1) is not None
        True
    """
    return get_fn_serial_c()
