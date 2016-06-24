/*
 * Form factor and scattering length tables
 * @author Tobias Weber
 * @date nov-2015
 * @license GPLv2
 */

#include "formfact.h"
#include "formfact_impl.h"
#include "libs/globals.h"


template class Formfact<t_real_glob>;
template class FormfactList<t_real_glob>;

template class MagFormfact<t_real_glob>;
template class MagFormfactList<t_real_glob>;

template class Scatlen<t_real_glob>;
template class ScatlenList<t_real_glob>;
