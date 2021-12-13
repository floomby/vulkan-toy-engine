### Know bugs which need fixing

 * Both the gui thread and the compute thread are using the same queue at the same time at some point, probably just can fix this by adding a seperate queue for just computing
 * I need to fix that broken pipe problem with the server when clients disconect
 * Redoing the gui texture references while drawing needs to be done, I am crashing all over the place cause of it, I think I know how to do it correctly