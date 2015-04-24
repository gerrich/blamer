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
import subprocess
import backend
import difflib
import re

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

  def get_runs_path(self, contest_id):
    return "/home/ejudge/ejudge-home/judges/%06d/var/archive/runs/"%(int(contest_id))

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
  def list(self, contest_id="26"):
    file_list=[]
    for root, dirs, files in os.walk(self.get_runs_path(contest_id)):
      for file in files:
        path = os.path.join(root, file)
        basename = os.path.basename(file)
        runid = re.sub('\..*$','', basename)
        try:
          runid = str(int(runid))
        except e:
          pass
        file_list.append([path, runid])

    template = self.env.get_template('list.html')
    return template.render(files=file_list)

  @cherrypy.expose
  def diff(self, contest_id="26", runid_a="", runid_b=""):
    data_a, encoding_a, fname_a = self.get_data_by_runid(contest_id, int(runid_a))
    data_b, encoding_b, fname_b = self.get_data_by_runid(contest_id, int(runid_b))
   
    differ = difflib.HtmlDiff(tabsize = 2)
    table = differ.make_table(data_a.split('\n'), data_b.split('\n'))

    template = self.env.get_template('diff.html')
    return template.render(
        data_a=data_a,
        data_b=data_b,
        table=table)

  @cherrypy.expose
  def file(self, runid="", contest_id="26"):
    data, encoding, fname = self.get_data_by_runid(contest_id, int(runid))

    #shingles = []
    shingles = shingle.make_shingles(data.split(u"\n"))
    #look_shingles(shingles)
    ans = backend.ask_shingles(backend.filter_records(shingles))
    sources = []
    backend.parse_tsv_response(ans, shingles, sources)
    sources_ans = backend.ask_audit([p[0] for p in sources])
    backend.process_audit_response(sources_ans, sources)

    #return "[" + runid + "] %s/%s/%s"%(a4,a3,a2)
    template = self.env.get_template('data.html')
    return template.render(
        data = data,
        encoding=encoding,
        fname = fname,
        runid=runid,
        shingles = shingles,
        comment="type(data): %s len(ans): %d => %s"%(type(data), len(ans), ans[:1000] +  "\n >>>"+ " ".join([r['value'] for r in shingles])),
        sources = sources)

  def get_data_by_runid(self, contest_id, id):
    codes=list('0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ')
    a2 = codes[(id >> 5)%32]
    a3 = codes[(id >> 10)%32]
    a4 = codes[(id >> 15)%32]
    path = self.get_runs_path(contest_id) + "%s/%s/%s/%06d"%(a4,a3,a2,id)

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
    return data,encoding,fname



cherrypy.config.update({'engine.autoreload.on': False})
cherrypy.server.unsubscribe()
cherrypy.engine.start()

application = cherrypy.tree.mount(Root())
