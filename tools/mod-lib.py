def gen_lib(name, cfiles=None, subdir=None):
    if cfiles is None:
        cfiles = ((f'{name}.c', f'ec/{name}.h'),)
    target = {
            'name': name,
            'out': f'build/lib/lib{name}.a',
            'c-files': cfiles,
        }
    if subdir is not None:
        target['srcdir'] = f'lib/{name}'
        target['objdir'] = f'build/lib-obj/{subdir}'
        target['depsdir'] = 'include'
    return target

def module():
    return {
        'name': 'lib',
        'flags': ('BUILD_AR',),

        'srcdir': 'lib',
        'objdir': 'build/lib-obj',
        'depsdir': 'include',

        'cflags': '-ffreestanding -Iinclude',

        'targets': (
                gen_lib('crepe', cfiles=(
                    ('box.c', 'crepe/box.h'),
                    ('button.c', 'crepe/button.h'),
                    ('context.c', 'crepe/context.h'),
                    ('draw.c', 'crepe/draw.h'),
                    ('image.c', 'crepe/image.h'),
                    ('label.c', 'crepe/label.h'),
                    ('margin.c', 'crepe/margin.h'),
                    ('title.c', 'crepe/title.h'),
                    ('widget.c', 'crepe/widget.h'),
                ), subdir='crepe'),
                gen_lib('image'),
                gen_lib('sound'),
                gen_lib('wm'),
            )
        }
