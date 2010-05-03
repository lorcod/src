EnsureSConsVersion(0, 96)

import bldutil, configure, os, sys

if sys.version_info[:2] < (2, 7):
    import distutils.sysconfig as sysconfig
else:
    import sysconfig

env = Environment()

root = os.environ.get('RSFROOT',os.getcwd())

bindir = os.path.join(root,'bin')
libdir = os.path.join(root,'lib')
incdir = os.path.join(root,'include')
docdir = os.path.join(root,'doc')
spcdir = os.path.join(root,'spec')
mandir = os.path.join(root,'share','man')

##########################################################################
# CONFIGURATION
##########################################################################
opts = configure.options('config.py')
opts.Add('RSFROOT','RSF installation root',root)
opts.Update(env)

if not os.path.isfile('config.py'):
    conf = Configure(env,custom_tests={'CheckAll':configure.check_all})
    conf.CheckAll()
    env = conf.Finish()

Help(opts.GenerateHelpText(env,cmp))
opts.Save('config.py',env)
config = env.Command('config.py','configure.py','')
env.Precious(config)

Clean(config,['#/config.log','#/.sconf_temp','configure.pyc'])
env.Alias('config',config)

# Figuring out Python package installation directory
# This needs to be done because files all over the source dir need to be
# installed there (sfplot, the Python API, etc)

# Deduce installation directory name. Should this happen in configure.py?
std_pkgdir = os.path.join(sysconfig.get_python_lib(),'rsf')
pkgdir = std_pkgdir.replace(sysconfig.PREFIX,root,1)

##########################################################################
# CUSTOM BUILDERS
##########################################################################

env.Append(BUILDERS={'RSF_Include':bldutil.Header,
                     'RSF_Place':bldutil.Place,
                     'RSF_Pycompile':bldutil.Pycompile,
                     'RSF_Docmerge':bldutil.Docmerge},
           SCANNERS=[bldutil.Include])

##########################################################################
# FRAMEWORK BUILD
##########################################################################

Depends('bldutil.pyc','configure.pyc')
env.RSF_Pycompile('bldutil.pyc','bldutil.py')

system = filter(lambda x: x[0] != '.', os.listdir('system'))
user = filter(lambda x: x[0] != '.' and x != 'nobody', os.listdir('user'))
# Avoid crashing when user places some files in RSFSRC/user
user = filter(lambda x: os.path.isdir(os.path.join('user',x)), user)

SConscript(dirs='framework',name='SConstruct',
       exports='env root bindir libdir pkgdir docdir spcdir mandir system user')

##########################################################################
# API BUILD
##########################################################################
api = env.get('API',[])
if type(api) is str:
    api = [api]
api.insert(0,'c')

Default('build/include')
Default('build/lib')
for dir in map(lambda x: os.path.join('api',x), api):
    build = os.path.join('build',dir)
    BuildDir(build,dir)
    api_exports = 'env root libdir '
    if dir == 'api/python':
        api_exports += 'pkgdir'
    else:
        api_exports += 'incdir'
    SConscript(dirs=build,name='SConstruct',exports=api_exports)
    Default(build)

##########################################################################
# SYSTEM BUILD
##########################################################################
for dir in map(lambda x: os.path.join('system',x), system):
    build = os.path.join('build',dir)
    BuildDir(build,dir)
    SConscript(dirs=build,name='SConstruct',exports='env root bindir pkgdir')
    Default(build)

##########################################################################
# USER BUILD
##########################################################################
for dir in map(lambda x: os.path.join('user',x), user):
    build = os.path.join('build',dir)
    BuildDir(build,dir)
    SConscript(dirs=build,name='SConstruct',
        exports='env root bindir pkgdir')
    Default(build)

##########################################################################
# PLOT BUILD
##########################################################################
pdirs = ('lib','main','plplot','test')

for dir in map(lambda x: os.path.join('plot',x), pdirs):
    build = os.path.join('build',dir)
    BuildDir(build,dir)

    if dir in ('plot/main','plot/test'):
        plot_exports = 'env root bindir pkgdir'
    elif dir == 'plot/lib':
        plot_exports = 'env root libdir incdir pkgdir'
    elif dir == 'plot/plplot':
        plot_exports = 'env root libdir bindir pkgdir'
    SConscript(dirs=build,name='SConstruct', exports=plot_exports)
    Default(build)

##########################################################################
# PENS BUILD
##########################################################################
pdirs = ('fonts','include','utilities','genlib','main','docs','scripts')

for dir in map(lambda x: os.path.join('pens',x), pdirs):
    build = os.path.join('build',dir)
    BuildDir(build,dir)
    if dir == 'pens/main':
        pens_exports = 'env root pkgdir bindir'
    elif dir == 'pens/scripts':
        pens_exports = 'env bindir pkgdir'
    else:
        pens_exports = 'env root incdir libdir bindir'
    SConscript(dirs=build,name='SConstruct',
               exports=pens_exports)
    Default(build)

##########################################################################
# SU BUILD
##########################################################################
sudirs = ('lib','main','plot')

for dir in map(lambda x: os.path.join('su',x), sudirs):
    build = os.path.join('build',dir)
    BuildDir(build,dir)
    if dir in ('su/main','su/plot'):
        su_exports = 'env root pkgdir bindir'
    else:
        su_exports = 'env root libdir bindir incdir'
    SConscript(dirs=build,name='SConstruct',
               exports=su_exports)
    Default(build)

##########################################################################
# INSTALLATION
##########################################################################

rsfuser = os.path.join(pkgdir,'user')
env.Install(rsfuser,os.path.join('framework', 'py_pkg', '__init__.py'))

env.Alias('install',[incdir,bindir,libdir,rsfuser,docdir,spcdir,mandir])
env.Clean('install', rsfuser)
