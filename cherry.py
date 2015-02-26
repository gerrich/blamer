import cherrypy
import random

# Our CherryPy application
class Root(object):
    @cherrypy.expose
    def index(self):
        return "hello world"

    @cherrypy.expose
    def generate(self):
        return ''.join(random.sample(string.hexdigits, 8))

cherrypy.config.update({'engine.autoreload.on': False})
cherrypy.server.unsubscribe()
cherrypy.engine.start()

application = cherrypy.tree.mount(Root())
