import asyncio
from aiohttp import web
from aiohttp.errors import ClientDisconnectedError
import os.path
from threading import Thread
from time import sleep
from queue import Empty

from . import command_processor
from .packets import registered_packets, registered_commands

app = web.Application()
app["sockets"] = []
basepath = os.path.normpath(os.path.join(os.path.abspath(__file__), os.pardir))
webpath = os.path.join(basepath, "static")

wa_queue = None

@asyncio.coroutine
def wshandler(req):
    ws = web.WebSocketResponse()
    yield from ws.prepare(req)
    who = req.transport.get_extra_info("peername")
    print("Websocket connected to %s" % (who,))
    try:
        req.app["sockets"].append(ws)
        while True:
            msg = yield from ws.receive()
            if msg.type == web.WSMsgType.CLOSING:
                break
    except ClientDisconnectedError:
        pass
    except RuntimeError:
        pass
    finally:
        req.app["sockets"].remove(ws)
        who = req.transport.get_extra_info("peername")
        print("Websocket disconnected from %s" % (who,))
    return ws

@asyncio.coroutine
def index(req):
    page = "Placeholder"
    return web.Response(body=page.encode(), content_type="text/html")

@asyncio.coroutine
def sendcommand(req):
    yield from req.post()
    parent = req.POST.get("parent")
    name = req.POST.get("name")
    arg = req.POST.get("arg")
    command_processor.process(parent, name, arg)
    return web.Response(text="")

@asyncio.coroutine
def on_shutdown(app):
    for ws in app["sockets"]:
        yield from ws.close(code=999, message="Server shutdown")

def handle_packets():
    while True:
        try:
            frame = wa_queue.get_nowait()
            for ws in app["sockets"]:
                ws.send_json(data={'sid': frame.sid, 'data': frame.data})
        except Empty:
            sleep(0.01)
            pass
        except EOFError:
            return

def setup_server(queue):
    global wa_queue
    wa_queue = queue

    queue_handler = Thread(target=handle_packets)
    queue_handler.daemon = True
    queue_handler.start()

    app.router.add_route("GET", "/ws", wshandler)
    app.router.add_route("GET", "/", index)
    app.router.add_route("POST", "/command", sendcommand)
    app.router.add_static("/static", webpath)

    app.on_shutdown.append(on_shutdown)

    loop = asyncio.get_event_loop()
    handler = app.make_handler()
    coro = loop.create_server(handler, "localhost", 5000)
    loop.create_task(coro)
    app["handler"] = handler

def shutdown():
    print("HTTP Server shutting down")
    loop = asyncio.get_event_loop()
    loop.run_until_complete(app.shutdown())
    loop.run_until_complete(app["handler"].finish_connections(10.0))
    loop.run_until_complete(app.cleanup())
