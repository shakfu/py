#!/usr/bin/env python3

import logging
import os
import re
import shutil
import subprocess
import sys
import zipfile
from abc import ABC, abstractmethod
from pathlib import Path

# import glob

IGNORE_ERRORS = False

DEBUG = True
if DEBUG:
    LOG_LEVEL = logging.DEBUG
else:
    LOG_LEVEL = logging.INFO

LOG_FORMAT = '%(relativeCreated)-4d %(levelname)-5s: %(name)-10s %(message)s'
logging.basicConfig(level=LOG_LEVEL, format=LOG_FORMAT, stream=sys.stdout)

class DependencyManager:
    """Aggreggates, copies dylib dependencies and fixed references.

    target: dylib to made relocatable
    frameworks_dir: where target dylib will be copied to with copied dependents
    exec_ref: back ref for executable or plugin
    """

    def __init__(self, target, frameworks_dir='build', staticlibs_dir=None,
            exec_ref='@loader_path/../Frameworks'):
        self.target = target
        self.frameworks_dir = frameworks_dir
        self.staticlibs_dir = staticlibs_dir
        self.exec_ref = exec_ref
        self.install_names = {}
        self.deps = []
        self.dep_list = []

    def is_valid_path(self, dep_path):
        return (dep_path == '' or 
                dep_path.startswith('/opt/local/') or 
                dep_path.startswith('/usr/local/') or 
                dep_path.startswith('/User/'))

    def get_deps(self, target=None):
        if not target:
            target = self.target
        key = os.path.basename(target)
        self.install_names[key] = []
        result = subprocess.check_output(['otool', '-L', target])
        entries = [line.decode('utf-8').strip() for line in result.splitlines()]
        for entry in entries:
            match = re.match(r'\s*(\S+)\s*\(compatibility version .+\)$', entry)
            if match:
                path = match.group(1)
                (dep_path, dep_filename) = os.path.split(path)
                if self.is_valid_path(dep_path):
                    if dep_path == '':
                        path = os.path.join('/usr/local/lib', dep_filename)

                    dep_path, dep_filename = os.path.split(path)
                    item = (path, '@rpath/' + dep_filename)
                    self.install_names[key].append(item)
                    if path not in self.deps:
                        self.deps.append(path)
                        self.get_deps(path)

    def process_deps(self):
        for dep in self.deps:
            dep_path, dep_filename = os.path.split(dep)
            dest = os.path.join(self.frameworks_dir, dep_filename)
            self.dep_list.append([dep, '@rpath/' + dep_filename])

    def copy_dylibs(self):
        if not os.path.exists(self.frameworks_dir):
            os.mkdir(self.frameworks_dir)

        # cp target to frameworks_dir
        if os.path.dirname(self.target) != self.frameworks_dir:
            dest = os.path.join(self.frameworks_dir, os.path.basename(self.target))
            shutil.copyfile(self.target, dest)
            os.chmod(dest, 0o644)
            cmdline = ['install_name_tool', '-id', self.exec_ref, dest]
            err = subprocess.call(cmdline)
            if err != 0:
                raise RuntimeError("Failed to change '{0}' '{1}'".format(dest, self.exec_ref))

        # copy the rest
        for item in self.dep_list:
            orig_path, transformed = item
            dirname, dylib = os.path.split(orig_path)

            dest = os.path.join(self.frameworks_dir, dylib)

            if not os.path.exists(dest):
                shutil.copyfile(orig_path, dest)
                os.chmod(dest, 0o644)

    def change_install_names(self):
        for key in sorted(self.install_names.keys()):
            # print(key)
            # for i in self.install_names[key]:
            #     print('\t', i)
            # print()

            target = os.path.join(self.frameworks_dir, key)
            deps = self.install_names[key]
            for dep in deps:
                old, new = dep

                (old_name_path, old_name_filename) = os.path.split(old)
                if key == old_name_filename:
                    cmdline = ['install_name_tool', '-id', new, target]
                else:
                    cmdline = ['install_name_tool', '-change', old, new, target]

                err = subprocess.call(cmdline)
                if err != 0:
                    raise RuntimeError("Failed to change '{0}' to '{1}' in '{2}".format(old, new, target))

    def transform_exec(self, target):
        result = subprocess.check_output(['otool', '-L', target])
        entries = [line.decode('utf-8').strip() for line in result.splitlines()]
        for entry in entries:
            match = re.match(r'\s*(\S+)\s*\(compatibility version .+\)$', entry)
            if match:
                path = match.group(1)
                (dep_path, dep_filename) = os.path.split(path)
                if self.is_valid_path(dep_path):
                    if dep_path == '':
                        path = os.path.join('/usr/local/lib', dep_filename)

                    dep_path, dep_filename = os.path.split(path)

                    dest = os.path.join(self.exec_ref, dep_filename)
                    cmdline = ['install_name_tool', '-change', path, dest, target]
                    subprocess.call(cmdline)

    def copy_staticlibs(self):
        if not self.staticlibs_dir:
            raise Exception("must set 'staticlibs_dir parameter")
        for i in self.deps:
            head, tail = os.path.split(i)
            name = tail.rstrip('.dylib')
            if '.' in name:
                name = os.path.splitext(name)[0] + '.a'
            static = os.path.join(head, name)
            exists = os.path.exists(static)
            if exists:
                shutil.copyfile(static, os.path.join(self.staticlibs_dir, name))
            else:
                print("revise: not exists", static)

    def process(self):
        self.get_deps()
        self.process_deps()
        self.copy_staticlibs()
        self.copy_dylibs()
        self.change_install_names()
        self.transform_exec('./eg')

class Project:
    root = Path.cwd()
    source = root.parent / 'source'
    support = root.parent.parent / 'support'
    scripts = root / 'scripts'
    patch = root / 'patch'
    targets =  root / 'targets'
    build = targets / 'build'
    downloads = build / 'downloads'
    src = build / 'src'
    lib = build / 'lib'


class Builder(ABC):
    name: str
    version: str
    url_template: str
    depends_on: []

    def __init__(self, project, version=None, depends_on=None):
        self.project = project or Project()
        self.version = version or self.version
        self.depends_on = ([B(project) for B in depends_on] if depends_on else
                           [B(project) for B in self.depends_on])
        self.log = logging.getLogger(self.__class__.__name__)

    def __repr__(self):
        return f"<{self.__class__.__name__} '{self.name}-{self.version}'>"

    def __iter__(self):
        for dependency in self.depends_on:
            yield dependency
            for subdependency in iter(dependency):
                yield subdependency
    @property
    def ver(self):
        return  ".".join(self.version.split('.')[:2])

    @property
    def ver_nodot(self):
        return self.ver.replace('.', '')

    @property
    def name_version(self):
        return f'{self.name}-{self.version}'

    @property
    def name_ver(self):
        return f'{self.name.lower()}{self.ver}'

    @property
    def url(self):
        return Path(self.url_template.format(name=self.name, 
                                        version=self.version))
    @property
    def name_archive(self):
        return f'{self.name_version}.tgz'

    @property
    def download_path(self):
        return self.project.downloads / self.name_archive

    @property
    def src_path(self):
        return self.project.src / self.name_version

    @property
    def lib_path(self):
        return self.prefix

    @property
    def prefix(self):
        return self.project.lib / self.name.lower()

    @property
    def prefix_lib(self):
        return self.prefix / 'lib'

    @property
    def prefix_include(self):
        return self.prefix / 'include'

    @property
    def prefix_bin(self):
        return self.prefix / 'bin'


    def libs_static_exist(self):
        for lib in self.libs_static:
            if not (self.prefix_lib / lib).exists():
                return False
        return True

    def cmd(self, shellcmd, *args, **kwargs):
        os.system(shellcmd.format(*args, **kwargs))

    def chdir(self, path):
        os.chdir(path)

    def move(self, src, dst):
        shutil.move(src, dst)

    def copytree(self, src, dst):
        shutil.copytree(src, dst)

    def copyfile(self, src, dst):
        shutil.copyfile(src, dst)

    def remove(self, path):
        if path.is_dir():
            shutil.rmtree(path, ignore_errors=IGNORE_ERRORS)
        else:
            path.unlink(missing_ok=True)

    def reset(self):
        self.remove(self.src_path)
        self.remove(self.prefix) # aka self.prefix

    def download(self):
        "download target src"

    def build(self):
        "build target from src"


class OSXBuilder(Builder):
    mac_dep_target = '10.13'

    @property
    def dylib(self):
        return f'lib{self.name.lower()}{self.ver}.dylib'        

    def download(self):
        "download src"

        self.project.downloads.mkdir(parents=True, exist_ok=True)
        for dep in self.depends_on:
            dep.download()

        # download
        if not self.download_path.exists():
            self.log.info(f"downloading {self.download_path}")
            self.cmd(f'curl -L --fail {self.url} -o {self.download_path}')
     
        # unpack
        if not self.src_path.exists():
            self.log.info(f"unpacking {self.src_path}")
            self.cmd(f'tar -C {self.project.src} -xvf {self.download_path}')


class OpensslBuilder(OSXBuilder):
    name = 'openssl'
    version = '1.1.1g'
    url_template = 'https://www.openssl.org/source/{name}-{version}.tar.gz'
    depends_on = []
    libs_static = ['libssl.a', 'libcrypto.a']

    def build(self):
        if not self.libs_static_exist():
            self.chdir(self.src_path)
            self.cmd(f'./config no-shared no-tests --prefix={self.prefix}')
            self.cmd('make install_sw')
            self.chdir(self.project.root)

class Bzip2Builder(OSXBuilder):
    name = 'bzip2'
    version = '1.0.8'
    url_template = 'https://sourceware.org/pub/bzip2/{name}-{version}.tar.gz'
    depends_on = []
    libs_static = ['libbz2.a']

    def build(self):
        if not self.libs_static_exist():
            self.chdir(self.src_path)
            self.cmd(f'make install PREFIX={self.prefix}')
            self.chdir(self.project.root)



class XzBuilder(OSXBuilder):
    name = 'xz'
    version = '5.2.5'
    url_template = 'http://tukaani.org/xz/{name}-{version}.tar.gz'
    depends_on = []
    libs_static = ['libxz.a']

    def build(self):
        if not self.libs_static_exist():
            self.chdir(self.src_path)
            self.cmd(f"""MACOSX_DEPLOYMENT_TARGET={self.mac_dep_target} \
                ./configure --disable-shared --enable-static --prefix={self.prefix}""")
            self.cmd(f'make && make install')
            self.chdir(self.project.root)


class PythonBuilder(OSXBuilder):
    name = 'Python'
    version = '3.8.4'
    url_template = 'https://www.python.org/ftp/python/{version}/{name}-{version}.tgz'
    depends_on = [OpensslBuilder, Bzip2Builder, XzBuilder]
    suffix = ""
    setup_local = None
    patch = None

    def __init__(self, project=None, version=None, depends_on=None):
        super().__init__(project, version, depends_on)

        # dependency manager attributes (revise)
        self.install_names = {}
        self.deps = []
        self.dep_list = []

    # ------------------------------------------------------------------------
    # python properties

    @property
    def static_lib(self):
        return f'lib{self.name.lower()}{self.ver}.a'

    @property
    def python_lib(self):
        return self.prefix_lib / self.name_ver

    @property
    def site_packages(self):
        return self.python_lib / 'site-packages'

    @property
    def lib_dynload(self):
        return self.python_lib / 'lib-dynload'    

    # ------------------------------------------------------------------------
    # src-level operations

    def pre_process(self):
        "pre-build operations"

    def post_process(self):
        "post-build operations"

    def write_setup_local(self, setup_local=None):
        if not any([setup_local, self.setup_local]):
            return
        if not setup_local:
            setup_local = self.setup_local
        self.copyfile(self.project.patch / setup_local, 
                  self.src_path /'Modules' / 'Setup.local')

    def apply_patch(self, patch=None):
        if not any([patch, self.patch]):
            return
        if not patch:
            patch = self.patch
        self.cmd(f'patch -p1 < {self.project.patch}/{patch}')

    def install(self):
        self.reset()
        self.download()
        self.pre_process()
        self.build()
        self.post_process()

    def install_python_pkg(self):
        self.install_python()
        self.fix_python_dylib_for_pkg()


    def install_python_ext(self):
        self.install_python()
        self.fix_python_dylib_for_ext()

    # ------------------------------------------------------------------------
    # post-processing operations

    def is_valid_path(self, dep_path):
        return (dep_path == '' or 
                dep_path.startswith('/opt/local/') or 
                dep_path.startswith('/usr/local/') or 
                dep_path.startswith('/User/'))

    def get_deps(self, target=None):
        if not target:
            target = self.target
        key = os.path.basename(target)
        self.install_names[key] = []
        result = subprocess.check_output(['otool', '-L', target])
        entries = [line.decode('utf-8').strip() for line in result.splitlines()]
        for entry in entries:
            match = re.match(r'\s*(\S+)\s*\(compatibility version .+\)$', entry)
            if match:
                path = match.group(1)
                (dep_path, dep_filename) = os.path.split(path)
                if self.is_valid_path(dep_path):
                    if dep_path == '':
                        path = os.path.join('/usr/local/lib', dep_filename)
                    dep_path, dep_filename = os.path.split(path)
                    item = (path, '@rpath/' + dep_filename)
                    self.install_names[key].append(item)
                    if path not in self.deps:
                        self.deps.append(path)
                        self.get_deps(path)



    def recursive_clean(self, name, pattern):
        self.cmd(f'find {name} | grep -E "({pattern})" | xargs rm -rf')

    def clean_python_pyc(self, name):
        self.recursive_clean(name, r"__pycache__|\.pyc|\.pyo$")

    def clean_python_tests(self, name):
        self.recursive_clean(name, "tests|test")

    def rm_libs(self, names):
        for name in names:
            self.remove(self.python_lib / name)

    def rm_exts(self, names):
        for name in names:
            self.remove(self.python_lib / 'lib-dynload' /
                        f'{name}.cpython-{self.ver_nodot}-darwin.so')

    def rm_bins(self, names):
        for name in names:
            self.remove(self.prefix_bin / name)

    def clean_python_site_packages(self):
        self.remove(self.python_lib / 'site-packages')

    def remove_packages(self):
        self.rm_libs([
            f'config-{self.ver}{self.suffix}-darwin',
            'idlelib',
            'lib2to3',
            'tkinter',
            'turtledemo',
            'turtle.py',
            'ctypes',
            'curses',
            'ensurepip',
            'venv',
        ])

    def remove_extensions(self): pass

    def remove_binaries(self):
        self.rm_bins([
            f'2to3-{self.ver}',
            f'idle{self.ver}',
            f'easy_install-{self.ver}',
            f'pip{self.ver}',
            f'pyvenv-{self.ver}',
            f'pydoc{self.ver}',
            # f'python{self.ver}{self.suffix}',
            # f'python{self.ver}-config',
        ])

    def clean(self):
        self.clean_python_pyc(self.prefix)
        self.clean_python_tests(self.python_lib)
        self.clean_python_site_packages()

        for i in (self.python_lib / 'distutils' / 'command').glob('*.exe'):
            self.remove(i)

        self.remove(self.prefix_lib / 'pkgconfig')
        self.remove(self.prefix / 'share')

        self.remove_packages()
        self.remove_extensions()
        self.remove_binaries()

    def zip_lib(self):
        temp_lib_dynload = self.prefix_lib / 'lib-dynload'
        temp_os_py = self.prefix_lib / 'os.py'

        self.remove(self.site_packages)
        self.lib_dynload.rename(temp_lib_dynload)
        self.copyfile(self.python_lib / 'os.py', temp_os_py)

        zip_path = self.prefix_lib  / f'python{self.ver_nodot}'
        shutil.make_archive(zip_path, 'zip', self.python_lib)

        self.remove(self.python_lib)
        self.python_lib.mkdir()
        temp_lib_dynload.rename(self.lib_dynload)
        temp_os_py.rename(self.python_lib / 'os.py')
        self.site_packages.mkdir()

        
    def fix_python_dylib_for_pkg(self):
        self.chdir(self.prefix_lib)
        self.cmd(f'chmod 777 {self.dylib}')
        self.cmd(
            'install_name_tool -id '
            '@loader_path/../../../../support/{self.name}/lib/{self.dylib} '
            '${self.dylib}')
        self.chdir(self.root)

    def fix_python_dylib_for_ext(self):
        self.chdir(self.prefix_lib)
        self.cmd(f'chmod 777 {self.dylib}')
        self.cmd('install_name_tool -id @loader_path/{self.dylib} {self.dylib}')
        self.chdir(self.root)

    def pre_process(self):
        self.chdir(self.src_path)
        self.write_setup_local()
        self.apply_patch()
        self.chdir(self.project.root)

    def post_process(self):
        self.clean()
        self.zip_lib()
        # self.fix()
        # self.sign()


class StaticPythonBuilder(PythonBuilder):
    setup_local = 'setup-static-min2.local'
    patch = 'makesetup.patch'

    @property
    def prefix(self):
        name = f'{self.name.lower()}-static'
        return self.project.lib / name

    def build(self):
        for dep in self.depends_on:
            dep.build()

        self.chdir(self.src_path)
        self.cmd(f"""\
        ./configure MACOSX_DEPLOYMENT_TARGET={self.mac_dep_target} \
            --prefix={self.prefix} \
            --without-doc-strings \
            --enable-ipv6 \
            --without-ensurepip \
            --with-lto \
            --enable-optimizations
        """)
        self.cmd('make altinstall')
        self.chdir(self.project.root)


    def post_process(self):
        self.clean()
        self.zip_lib()
        self.static_lib.rename(self.prefix / self.library)



class SharedPythonBuilder(PythonBuilder):
    setup_local = 'setup-shared.local'

    @property
    def prefix(self):
        name = f'{self.name.lower()}-shared'
        return self.project.lib / name

    def build(self):
        for dep in self.depends_on:
            dep.build()

        self.chdir(self.src_path)
        self.cmd(f"""\
        ./configure MACOSX_DEPLOYMENT_TARGET={self.mac_dep_target} \
            --prefix={self.prefix} \
            --enable-shared \
            --with-openssl={self.project.lib / 'openssl'} \
            --without-doc-strings \
            --enable-ipv6 \
            --without-ensurepip \
            --with-lto \
            --enable-optimizations
        """)
        self.cmd('make altinstall')
        self.chdir(self.project.root)


    def remove_extensions(self):
        self.rm_exts([
            '_tkinter',
            '_ctypes',
            '_multibytecodec',
            '_codecs_jp',
            '_codecs_hk',
            '_codecs_cn',
            '_codecs_kr',
            '_codecs_tw',
            '_codecs_iso2022',
            '_curses',
            '_curses_panel',
        ])


class FrameworkPythonBuilder(PythonBuilder):
    setup_local = 'setup-shared.local'

    @property
    def prefix(self):
        return self.project.lib / 'Python.framework' / 'Versions' / self.ver

    def reset(self):
        self.remove(self.src_path)
        self.remove(self.project.lib / 'Python.framework')

    def build(self):
        for dep in self.depends_on:
            dep.build()

        self.chdir(self.src_path)
        self.cmd(f"""\
        ./configure MACOSX_DEPLOYMENT_TARGET={self.mac_dep_target} \
            --enable-framework={self.project.lib} \
            --with-openssl={self.project.lib / 'openssl'} \
            --without-doc-strings \
            --enable-ipv6 \
            --without-ensurepip \
            --with-lto \
            --enable-optimizations
        """)
        self.cmd('make altinstall')
        self.chdir(self.project.root)


    def remove_extensions(self):
        self.rm_exts([
            '_tkinter',
            '_ctypes',
            '_multibytecodec',
            '_codecs_jp',
            '_codecs_hk',
            '_codecs_cn',
            '_codecs_kr',
            '_codecs_tw',
            '_codecs_iso2022',
            '_curses',
            '_curses_panel',
        ])

    def post_process(self):
        self.clean()
        self.zip_lib()



if __name__ == '__main__':
    p = FrameworkPythonBuilder()
    p.install()
    # p.reset()
    # p.download()
    # p.build()
    # p.clean()
    # p.zip_lib()
