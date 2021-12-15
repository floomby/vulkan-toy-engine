### Know bugs which need fixing

 * Both the gui thread and the compute thread are using the same queue at the same time at some point, probably just can fix this by adding a seperate queue for just computing
 * Redoing the gui texture references while drawing needs to be done, I am crashing all over the place cause of it, I think I know how to do it correctly (I think this was a thread synchronization problem in the refence counting I fixed)
 * The targeting code in pathgen.hpp is wrong (It correctly does the wrong thing)