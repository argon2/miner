/*
 * Copyright (c) 2014-2017 BM-2cVZag8xxXdPC9BsLewmUotg6TBB8T2yTk
 *
 * This file is part of Miner++.
 *
 * Miner++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MINER_MINER_HPP
#define MINER_MINER_HPP

/**
 * Overview:
 * stratum <== hello
 * ==> work manager ==> notify all
 * cpu handler ==> write
 * gpu handler ==> write
 * scan hash
 * cpu ==> read
 * gpu ==> read
 * serial device ==> read
 * check work ? work manager ==> stratum ==> mining.submit
 */

namespace miner {

} // namespace miner

#endif // MINER_MINER_HPP
