#!/usr/bin/env python
#
#   Copyright 2011 Matteo Bertozzi
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import commands
import shutil
import string
import sys
import os

def execCommand(cmd):
    return commands.getstatusoutput(cmd)

def writeFile(filename, content):
    fd = open(filename, 'w')
    try:
        fd.write(content)
    finally:
        fd.close()

def _removeDirectory(path):
    if os.path.exists(path):
        for root, dirs, files in os.walk(path, topdown=False):
            for name in files:
                os.remove(os.path.join(root, name))
            for name in dirs:
                os.rmdir(os.path.join(root, name))
        os.removedirs(path)

def removeDirectoryContents(path):
    for root, dirs, files in os.walk(path, topdown=False):
        for name in dirs:
            _removeDirectory(os.path.join(root, name))

def _findCompiler():
    paths = os.getenv('PATH', '/usr/bin').split(':')
    compilers = ['clang', 'gcc']

    for compiler in compilers:
        for path in paths:
            compiler_path = os.path.join(path, compiler)
            if os.path.exists(compiler_path):
                return compiler_path

    raise Exception('Compiler Not Found!')

def ldLibraryPathUpdate(ldlibs):
    env_name = 'LD_LIBRARY_PATH'
    env = os.environ.get(env_name, '')
    env_libs = ':'.join([lib for lib in ldlibs if lib not in env])
    if env_libs:
        env = '%s:%s' % (env, env_libs) if env else env_libs
        os.environ[env_name] = env

class BuildOptions(object):
    def __init__(self):
        self.cc = _findCompiler()
        self.ldlibs = []
        self.cflags = []
        self.defines = []
        self.includes = []

    def setCompiler(self, cc):
        self.cc = cc

    def addCFlags(self, cflags):
        self.cflags.extend(cflags)

    def addDefines(self, defines):
        self.defines.extend(defines)

    def addIncludePaths(self, includes):
        self.includes.extend(includes)

    def addLdLibs(self, ldlibs):
        self.ldlibs.extend(ldlibs)

    def clone(self):
        opts = BuildOptions()
        opts.setCompiler(self.cc)
        opts.addCFlags(self.cflags)
        opts.addDefines(self.defines)
        opts.addIncludePaths(self.includes)
        opts.addLdLibs(self.ldlibs)
        return opts

class Build(object):
    DEFAULT_BUILD_DIR = 'build'

    def __init__(self, name, build_dir=None, options=[]):
        def _setDefaultFunc(value, default_f):
            return value if value else default_f()

        def _setDefaultValue(value, default_v):
            return value if value else default_v

        self.name = name
        self._options = _setDefaultFunc(options, BuildOptions)
        self._makeBuildDirs(_setDefaultValue(build_dir, self.DEFAULT_BUILD_DIR))

    def build(self, *args, **kwargs):
        self.cleanup()
        os.makedirs(self._dir_obj)
        os.makedirs(self._dir_lib)
        os.makedirs(self._dir_inc)
        return self._build(*args, **kwargs)

    def _build(self):
        raise NotImplementedError

    def cleanup(self, full=True):
        if full:
            _removeDirectory(self._dir_out)
        else:
            _removeDirectory(self._dir_obj)

    def runTools(self, name, tools, verbose=True):
        print 'Run Tools:', name
        print '-' * 60

        ldLibraryPathUpdate([self._dir_lib])

        for tool in tools:
            exit_code, output = execCommand(tool)

            if verbose:
                print output

            if exit_code != 0:
                print ' [FAIL]', tool, 'exit code', exit_code
            else:
                print ' [ OK ]', tool
        print

    def _makeBuildDirs(self, build_dir):
        self._file_buildnr = os.path.join(build_dir, '.buildnr-%s' % self.name)
        self._dir_out = os.path.join(build_dir, self.name)
        self._dir_obj = os.path.join(self._dir_out, 'objs')
        self._dir_lib = os.path.join(self._dir_out, 'libs')
        self._dir_inc = os.path.join(self._dir_out, 'include')

    def compileFile(self, filename, dump_error=True):
        obj_name, obj_path = self._objFilePath(filename)

        cmd = '%s -c %s %s %s %s %s -o %s' %                \
               (self._options.cc,                           \
                string.join(self._options.cflags, ' '),     \
                string.join(self._options.defines, ' '),    \
                string.join(self._options.includes, ' '),   \
                '-I%s' % self._dir_inc,                     \
                filename,                                   \
                obj_path)

        print ' [CC]', filename
        exit_code, output = execCommand(cmd)
        if exit_code != 0:
            if dump_error:
                print ' * Failed with Status', exit_code
                print ' * %s' % (cmd)
                print output
            raise RuntimeError("Compilation Failure!")

    def compileDirectory(self, dir_src):
        return self._cFilesWalk(dir_src, self.compileFile)

    def linkFile(self, filename, dump_error=True):
        obj_name, obj_path = self._objFilePath(filename)
        app_name, app_path = self._appFilePathFromObj(obj_path)

        cmd = '%s -o %s %s %s' %                            \
                (self._options.cc, app_path, obj_path,      \
                 string.join(self._options.ldlibs, ' '))
        print ' [LD]', app_name
        exit_code, output = execCommand(cmd)
        if exit_code != 0:
            if dump_error:
                print ' * Failed with Status', exit_code
                print ' *', cmd
                print output
            raise RuntimeError("Linking Failure!")

    def _objFilePath(self, filename):
        objname = os.path.normpath(filename).replace('/', '_')
        objname = objname[:objname.rindex('.')] + '.o'
        objpath = os.path.join(self._dir_obj, objname)
        return objname, objpath

    def _appFilePath(self, filename):
        obj_path = self._objFilePath(filename)
        return self._appFilePathFromObj(obj_path)

    def _appFilePathFromObj(self, obj_path):
        app_name = obj_path[obj_path.rfind('_') + 1:-2]
        app_path = os.path.join(self._dir_out, app_name)
        return app_name, app_path

    def _cFilesWalk(self, dir_src, func):
        count = 0
        for root, dirs, files in os.walk(dir_src, topdown=False):
            for name in files:
                if name.endswith('.c'):
                    func(os.path.join(root, name))
                    count += 1
        return count

    def buildNumber(self, major, minor, inc=1):
        mx = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'

        try:
            build = int(file(self._file_buildnr, 'r').read()) + inc
        except:
            build = 0

        if inc > 0:
            file(self._file_buildnr, 'w').write('%d' % build)

        return '%d%s%04X' % (major, mx[minor], build)

    @staticmethod
    def platformIsMac():
        return os.uname()[0] == 'Darwin'

    @staticmethod
    def platformIsLinux():
        return os.uname()[0] == 'Linux'

class BuildApp(Build):
    def __init__(self, name, src_dirs, **kwargs):
        super(BuildApp, self).__init__(name, **kwargs)
        self.src_dirs = src_dirs

    def _build(self):
        print 'Building %s' % (self.name)
        print '-' * 60

        compiled_files = 0
        for dir_src in self.src_dirs:
            compiled_files += self.compileDirectory(dir_src)

        if not compiled_files:
            return

        obj_list = os.listdir(self._dir_obj)
        obj_list = [os.path.join(self._dir_obj, f) for f in obj_list]

        app_path = os.path.join(self._dir_out, self.name)

        cmd = '%s -o %s %s %s' %                            \
                (self._options.cc, app_path,                \
                 string.join(obj_list, ' '),                \
                 string.join(self._options.ldlibs, ' '))

        print ' [LD]', self.name
        exit_code, output = execCommand(cmd)
        if exit_code != 0:
            print ' * Failed with Status', exit_code
            print ' *', cmd
            print output
            sys.exit(1)
        print

class BuildMiniTools(Build):
    def __init__(self, name, src_dirs, **kwargs):
        super(BuildMiniTools, self).__init__(name, **kwargs)
        self.src_dirs = src_dirs

    def _build(self):
        print 'Building %s' % (self.name)
        print '-' * 60

        for src_dir in self.src_dirs:
            if not self.compileDirectory(src_dir):
                return []

        tools = []
        for obj_name in os.listdir(self._dir_obj):
            app_name = obj_name[obj_name.rfind('_') + 1:-2]
            app_path = os.path.join(self._dir_out, app_name)
            obj_path = os.path.join(self._dir_obj, obj_name)

            cmd = '%s -o %s %s %s' %                            \
                    (self._options.cc, app_path, obj_path,      \
                     string.join(self._options.ldlibs, ' '))
            print ' [LD]', app_name
            exit_code, output = execCommand(cmd)
            if exit_code != 0:
                print ' * Failed with Status', exit_code
                print ' *', cmd
                print output
                sys.exit(1)

            tools.append(app_path)

        print
        return sorted(tools)

class BuildConfig(Build):
    def __init__(self, name, src_dirs, **kwargs):
        super(BuildConfig, self).__init__(name, **kwargs)
        self.src_dirs = src_dirs

    def _build(self, config_file, config_head, dump_error=False):
        print 'Running Build.Config'
        print '-' * 60

        config = []
        def _testApp(filename):
            try:
                self.compileFile(filename, dump_error=dump_error)
                self.linkFile(filename, dump_error=dump_error)
            except:
                print ' [!!]', filename
                return

            obj_name, obj_path = self._objFilePath(filename)
            app_name, app_path = self._appFilePathFromObj(obj_path)

            ldLibraryPathUpdate([self._dir_lib])
            exit_code, output = execCommand(app_path)
            if exit_code != 0:
                print ' [!!]', filename
                return

            config.append(app_name)

        for src_dir in self.src_dirs:
            self._cFilesWalk(src_dir, _testApp)

        print ' [WR] Write config', config_file
        fd = open(config_file, 'w')
        fd.write('#ifndef _%s_BUILD_CONFIG_H_\n' % config_head)
        fd.write('#define _%s_BUILD_CONFIG_H_\n' % config_head)
        fd.write('\n')
        for define in config:
            fd.write('#define %s_%s\n' % (config_head, define.upper().replace('-', '_')))
        fd.write('\n')
        fd.write('#endif /* !_%s_BUILD_CONFIG_H_ */\n' % config_head)
        fd.close()

        print

class BuildLibrary(Build):
    SKIP_HEADER_ENDS = ('_p.h', 'private.h')

    def __init__(self, name, version, src_dirs, copy_dirs=None, **kwargs):
        super(BuildLibrary, self).__init__(name, **kwargs)
        self.version = version
        self.src_dirs = src_dirs
        self.copy_dirs = copy_dirs or []

    def moveBuild(self, dst):
        _removeDirectory(dst)
        os.makedirs(dst)
        os.rename(self._dir_inc, os.path.join(dst, 'include'))
        os.rename(self._dir_lib, os.path.join(dst, 'libs'))

    def _build(self):
        v_maj, v_min, v_rev = (int(x) for x in self.version.split('.'))
        build_nr = self.buildNumber(v_maj, v_min)

        print 'Copy %s %s (%s) Library Headers' % (self.name, self.version, build_nr)
        print '-' * 60
        self.copyHeaders()

        print 'Building %s %s (%s) Library' % (self.name, self.version, build_nr)
        print '-' * 60

        compiled_files = 0
        for dir_src in self.src_dirs:
            compiled_files += self.compileDirectory(dir_src)

        if not compiled_files:
            return

        obj_list = os.listdir(self._dir_obj)
        obj_list = [os.path.join(self._dir_obj, f) for f in obj_list]

        if not os.path.exists(self._dir_lib):
            os.makedirs(self._dir_lib)

        libversion_maj = self.version[:self.version.index('.')]
        lib_ext = 'dylib' if self.platformIsMac() else 'so'
        lib_name = 'lib%s.%s' % (self.name, lib_ext)
        lib_name_maj = 'lib%s.%s.%s' % (self.name, lib_ext, libversion_maj)
        lib_name_full = 'lib%s.%s.%s' % (self.name, lib_ext, self.version)
        lib_path = os.path.join(self._dir_lib, lib_name_full)

        if self.platformIsMac():
            cmd = '%s -dynamiclib -current_version %s -o %s -dylib %s %s' % \
                    (self._options.cc, self.version, lib_path,              \
                     string.join(obj_list, ' '),                            \
                     string.join(self._options.ldlibs, ' '))
        elif self.platformIsLinux():
            cmd = '%s -shared -Wl,-soname,%s -o %s %s %s' %                 \
                    (self._options.cc, lib_name_maj, lib_path,              \
                     string.join(obj_list, ' '),                            \
                     string.join(self._options.ldlibs, ' '))
        else:
            raise RuntimeError("Unsupported Platform %s" % ' '.join(os.uname()))

        print
        print ' [LD]', lib_name_full
        exit_code, output = execCommand(cmd)
        if exit_code != 0:
            print ' * Failed with Status', exit_code
            print ' *', cmd
            print output
            sys.exit(1)

        cwd = os.getcwd()
        os.chdir(self._dir_lib)
        for name in (lib_name, lib_name_maj):
            print ' [LN]', name
            execCommand('ln -s %s %s' % (lib_name_full, name))
        os.chdir(cwd)

        print

    def copyHeaders(self):
        dir_dst = os.path.join(self._dir_inc, self.name)
        self.copyHeadersFromTo(None, self.src_dirs)
        for hname, hdirs in self.copy_dirs:
            self.copyHeadersFromTo(hname, hdirs)
        print

    def copyHeadersFromTo(self, name, src_dirs):
        dir_dst = os.path.join(self._dir_inc, self.name)
        if name is not None:
            dir_dst = os.path.join(dir_dst, name)
        _removeDirectory(dir_dst)
        os.makedirs(dir_dst)

        for dir_src in src_dirs:
            for root, dirs, files in os.walk(dir_src, topdown=False):
                for name in files:
                    if not name.endswith('.h'):
                        continue

                    for s in self.SKIP_HEADER_ENDS:
                        if name.endswith(s):
                            break
                    else:
                        src_path = os.path.join(root, name)
                        dst_path = os.path.join(dir_dst, name)
                        shutil.copyfile(src_path, dst_path)
                        print ' [CP]', dst_path

def _parseCmdline():
    try:
        from argparse import ArgumentParser
    except ImportError:
        from optparse import OptionParser
        class ArgumentParser(OptionParser):
            def add_argument(self, *args, **kwargs):
                return self.add_option(*args, **kwargs)

            def parse_args(self):
                options, args = OptionParser.parse_args(self)
                return options

    parser = ArgumentParser()
    parser.add_argument('-c', '--compiler', dest='compiler', action='store',
                        help='Compiler to use')

    parser.add_argument('leveldb', metavar='leveldb-path',
                        help="LevelDB Path")

    return parser.parse_args()

if __name__ == '__main__':
    options = _parseCmdline()

    DEFAULT_RELEASE_CFLAGS = ['-O3', '-Wall', '-Werror']
    DEFAULT_DEBUG_CFLAGS = ['-g', '-Wall', '-Werror']
    DEFAULT_DEFINES = ['-D__USE_FILE_OFFSET64']
    DEFAULT_LDLIBS = ['-lpthread']

    # Default Build Options
    default_opts = BuildOptions()
    default_opts.setCompiler('g++')
    default_opts.addDefines(DEFAULT_DEFINES)
    default_opts.addLdLibs(DEFAULT_LDLIBS)

    # LevelDB Path
    leveldb_ldlibs = ['-L%s -lleveldb' % options.leveldb]
    leveldb_includes = ['-I%s' % os.path.join(options.leveldb, 'include')]

    # Update current LD_LIBRARY_PATH
    ldLibraryPathUpdate(['../../build/zcl/libs/', options.leveldb])

    # Compute ld libs/includes path
    zcl_ldlibs = ['-L../../build/zcl/libs/ -lzcl']
    zcl_includes = ['-I../../build/zcl/include/']

    build_opts = default_opts.clone()
    build_opts.addLdLibs(zcl_ldlibs)
    build_opts.addLdLibs(leveldb_ldlibs)
    build_opts.addIncludePaths(zcl_includes)
    build_opts.addIncludePaths(leveldb_includes)

    build = BuildApp('demo-leveldb', ['.'], options=build_opts)
    build.build()
