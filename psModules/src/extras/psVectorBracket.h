/* @file  psVectorBracket.h
 * @brief vector bracket functions
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-01-24 02:54:15 $
 * Copyright 2004-2005 Institute for Astronomy, University of Hawaii
 */

# ifndef PS_VECTOR_BRACKET_H
# define PS_VECTOR_BRACKET_H

/// @addtogroup Extras Miscellaneous Funtions
/// @{

int psVectorBracket(const psVector *index, psF32 key, bool above);
int psVectorBracketDescend(const psVector *index, psF32 key, bool above);
psF32 psVectorInterpolate(const psVector *index, const psVector *value, psF32 key);

/// @}
# endif /* PS_VECTOR_BRACKET_H */
