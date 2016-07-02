/*
 * Copyright 2016 Nils Christopher Brause
 *
 * This file is part of libqre.
 *
 * libqre is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libqre is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libqre.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <qre.hpp>

qre::qre(const std::string &str)
  : id(0)
{
  std::list<symbol> syms = tokeniser(str);
  the_chain = parse_expression(syms);

  if(!the_chain)
    throw std::runtime_error("Expected expression.");
  if(syms.size() > 0)
    throw std::runtime_error("Unparsed tokens.");
}
