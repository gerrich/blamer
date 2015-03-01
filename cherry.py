# -*- coding: utf8 -*-

import sys
sys.path.append('/home/ejudge/blamer')
import cherrypy
import random
import string
import dircache
import os
from jinja2 import Environment, BaseLoader, FileSystemLoader
import gzip
import shingle

def decode_smart(data):
  for encoding in ['utf8','cp1251']:
    try:
      return data.decode(encoding), encoding
    except:
      pass
  return data, None

# Our CherryPy application
class Root(object):
  def __init__(self):
    self.env = Environment(loader=FileSystemLoader('/home/ejudge/blamer'))
    self.runs_path = "/home/ejudge/ejudge-home/judges/000024/var/archive/runs/"

  @cherrypy.expose
  def index(self):
    return "hello world"

  @cherrypy.expose
  def generate(self):
    return ''.join(random.sample(string.hexdigits, 8))

  @cherrypy.expose
  def _list(self):
    items = dircache.listdir('/home/ejudge/ejudge-home/judges/000024/var/archive/')
    return "items " + str(items)
  
  @cherrypy.expose
  def list(self):
    file_list=[]
    for root, dirs, files in os.walk(self.runs_path):
      for file in files:
        file_list.append(os.path.join(root, file))
        #if file.endswith(".txt"):
        #  print(os.path.join(root, file))
    template = self.env.get_template('list.html')
    return template.render(files=file_list)

  @cherrypy.expose
  def file(self, runid=""):
    id=int(runid)
    codes=list('0123456789ABCDEFGHIJKLMNOPQRTSTUVWXYZ')
    a2 = codes[(id >> 5)%32]
    a3 = codes[(id >> 10)%32]
    a4 = codes[(id >> 15)%32]
    path = self.runs_path + "%s/%s/%s/%06d"%(a4,a3,a2,id)

    data='UNDEFINED'
    fname=path
    try:
      f=open(path,'r')
      data=f.read()
      f.close()
    except:
      fname=path + '.gz'
      f=gzip.open(fname, 'rb')
      data=f.read()
      f.close()

    data,encoding = decode_smart(data)
    shingles = shingle.make_shingles(data.split("\n"))

    #return "[" + runid + "] %s/%s/%s"%(a4,a3,a2)
    template = self.env.get_template('data.html')
    return template.render(data = data, encoding=encoding, fname = fname, shingles = shingles)

cherrypy.config.update({'engine.autoreload.on': False})
cherrypy.server.unsubscribe()
cherrypy.engine.start()

application = cherrypy.tree.mount(Root())
