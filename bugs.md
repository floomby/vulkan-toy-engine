### Know bugs which need fixing

 * Both the gui thread and the compute thread are using the same queue at the same time at some point, probably just can fix this by adding a seperate queue for just computing
 * Somehow the lua stack is getting corrupted when doing startup for the gui thread