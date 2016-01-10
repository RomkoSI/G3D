#!/usr/bin/env python
from __future__ import print_function

# regconfig.py
import winreg
import os.path as path

class RegConfig:
    """ Class that encapsulates a registry (path) as a configuration store.

        section = registry key
        option = registry valuename
        value = registry value

        Storage model = dictionary

    """

    def __init__(self, reg_path=None, autowrite=False):
        
        self._regpath = reg_path
        self._autowrite = autowrite
        self._store = {}

        # Autowrite is available only if a registry path is provided.
        if not self._regpath: self._autowrite = False

        # read the registry and store values if path given.
        if self._regpath: self.read(self._regpath)


    def add_section(self, section):
        """ Add a section named section to the instance. If a section by the
            given name already exists, ValueError is raised. """
        if not self.has_section(section):
            self._store[section] = {}
        else:
            raise ValueError("Section already exists")
        
    def get(self, section, option):
        """ Get an option value for the named section. """
        if self.has_option(section, option):
            return self._store[section][option][0]

                
    def _get_default_regkey(self, regpath=None, forwriting=False):
        """ Get the registry key handle for registry operations. provided the
            registry path. """ 
        if regpath:
            key = regpath
            subkey = ''
            while path.split(key)[0]:
                key, tmp = path.split(key)
                subkey = '\\'.join([tmp, subkey])              
            if key == 'HKEY_CLASSES_ROOT':
                key = winreg.HKEY_CLASSES_ROOT
            elif key == 'HKEY_CURRENT_CONFIG':
                key = winreg.HKEY_CURRENT_CONFIG
            elif key == 'HKEY_CURRENT_USER':
                key = winreg.HKEY_CURRENT_USER
            elif key == 'HKEY_DYN_DATA':
                key = winreg.HKEY_DYN_DATA
            elif key == 'HKEY_LOCAL_MACHINE':
                key = winreg.HKEY_LOCAL_MACHINE
            elif key == 'HKEY_PERFORMANCE_DATA':
                key = winreg.HKEY_PERFORMANCE_DATA
            elif key == 'HKEY_USERS':
                key = winreg.HKEY_USERS
            else:
                raise TypeError('Invalid registry key (HKEY_)')
            try:
                if forwriting:
                    hkey = winreg.CreateKey(key, subkey)
                else:
                    hkey = winreg.OpenKey(key, subkey)
            except:
                raise WindowsError('Cannot open registry path')
            else:
                return hkey
        
                    
    def items(self, section):
        """ Return a list of (name, value) pairs for each option in the given
            section. """
        if self.has_section(section):
            sectiondict = self._store[section]
            ret = []
            for option in list(sectiondict.keys()):
                value, type = sectiondict[option]
                ret.append((option, value))
            return ret
        
        
    def has_option(self, section, option):
        """ If the given section exists, and contains the given option. """
        if section in self._store:
            sectiondict = self._store[section]
            return option in sectiondict
    
    
    def has_section(self, section):
        """ Indicates whether the named section is present in the
            configuration. """
        return section in self._store
        
        
    def options(self, section):
        """ Returns a list of options available in the specified section. """
        if section in self._store:
            sectiondict = self._store[section]
            if sectiondict:
                return list(sectiondict.keys())
            
            
    def read(self, regpath=None):
        """ Can be a single or a list of registry paths. When multiple registry
            keys are read, options and values are overwritten in a FIFO
            fashion ordered from the list. """
        if regpath:
            try:
                if isinstance(regpath, list):
                    for rp in regpath:
                        self._read(self._get_default_regkey(rp))
                else:
                    self._read(self._get_default_regkey(regpath))
            except Exception as e:
                if not self._autowrite: raise e
                
    def _read(self, regkey=None):
        """ Parses a registry path into sections, options and values. """
        if regkey:
            index = 0
            while 1:
                try:
                    section = winreg.EnumKey(regkey, index)
                except: 
                    break
                else:
                    try:
                        self.add_section(section)
                    except:
                        pass
                    index += 1
                    
            for section in self.sections():
                index = 0
                while 1:
                    try:
                        hkey = winreg.OpenKey(regkey, section)   
                        valuename, data, type = winreg.EnumValue(hkey, index)
                    except:
                        break
                    else:
                        self.set(section, valuename, (data, type), readop=True)
                        index += 1
                        if hkey: winreg.CloseKey(hkey)
            if regkey: winreg.CloseKey(regkey)
                        
                        
    def remove_option(self, section, option):
        if self.has_option(section, option):
            del self._store[section][option]
            
                     
    def remove_section(self, section):
        if self.has_section(section):
            del self._store[section]     


    def sections(self):
        """ Return a list of the sections available. """
        return list(self._store.keys())
    

    def set(self, section, option, value, check=True, readop=False):
        """ If the given section exists, set the given option to the specified
            value; if check=True ValueError is raised if it nonexistent
            otherwise the section is written. """
        if not self.has_section(section):
            if check and not self._autowrite:
                raise ValueError('Section does not exist')
            self.add_section(section)
            
        sectiondict = self._store[section]
        
        if isinstance(value, tuple):
            pass
        else:
            value = self._valueregform(value)
            
        # Automatically write to rgistry if specified.
        if self._autowrite and not readop:
            self._write_reg(self._regpath, section, option, value)
                    
        
        sectiondict[option] = value
        

    def _valueregform(self, value):
        """ Converts values set by the user to a from to be stored in the
            registry. """
        if isinstance(value, str) or isinstance(value, str):
            value = (value, winreg.REG_SZ)
        elif isinstance(value, int):
            value = (value, winreg.REG_DWORD)
        elif isinstance(value, list):
            for s in value:
                if not isinstance(s, str) or isinstance(s, str):
                    raise TypeError(
                        "Values in list must be of type str or unicode.")
            value = (value, winreg.REG_MULTI_SZ)
        else:
            raise TypeError('Invalid value')
    
        return value
    
    def write(self, regpath=None):
        """ Writes configuration to the registry path. """
        if not regpath:
            regpath = self._regpath
        if regpath:
            for section in self.sections():
                sectiondict = self._store[section]
                for option in list(sectiondict.keys()):
                    self._write_reg(*(regpath, section, option,
                                            sectiondict[option]))
        
        
    def _write_reg(self, regpath, section, option, value):
        """ Writes to the registry path. """
        hkey = winreg.CreateKey(self._get_default_regkey(regpath, True),
                                 section)
        winreg.SetValueEx(hkey, option, 0, value[1], value[0])
        if hkey: winreg.CloseKey(hkey)


if __name__ == '__main__':

    rc = RegConfig(r"HKEY_LOCAL_MACHINE\SOFTWARE\mysoftware", autowrite=True)
    rc.set('configuration', 'configparser', 0)
    rc.set('configuration', 'regconfig', 'it works')
    rc.set('settings', 'users', ['jack', 'john', 'who else?'])
    
    # Can u store a pickle? yes...however...
    """ Quote - Value lengths are limited by available memory. Long values
        (more than 2048 bytes) should be stored as files with the filenames
        stored in the configuration registry. This helps the registry perform
        efficiently."""
    import pickle
    x = {'hi': 'im going to be pickled...' }
    pick = pickle.dumps(x, pickle.HIGHEST_PROTOCOL)
    rc.set('pickle', 'pickleobject', (pick, winreg.REG_BINARY))
    rc.write()
    
    #get sections and items
    for section in rc.sections():
        print(section)
        for item in rc.items(section):
            print('\t' + item)
            
    # Call this to write to registry path use it to configure different users..
    rc.write(r"HKEY_LOCAL_MACHINE\SOFTWARE\mysoftwareagain")
    
    # let's try reading the data only
    rc = RegConfig(r"HKEY_LOCAL_MACHINE\SOFTWARE\mysoftware")
    
    # let unpickle the pickle
    pick = rc.get('pickle', 'pickleobject')
    print(pickle.loads(pick))


