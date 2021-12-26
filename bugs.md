### Know bugs which need fixing

 * There might still be a race somewhere in the gui code (I sometimes get a corrupted vertex buffer for one of the buffers)
 * I had a lua gui thread stack out of space error (I have been unable to recreate it)
 * The targeting code in pathgen.hpp is wrong