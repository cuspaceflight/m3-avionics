from toad_backend import run
import multiprocessing
com_proc_pipe, frontend_pipe = multiprocessing.Pipe(True)

run(frontend_pipe)
