@rem Set file associations
set program="%~dp0%viewer64.exe"

assoc .obj=G3D.Viewer.10
assoc .ifs=G3D.Viewer.10
assoc .3ds=G3D.Viewer.10
assoc .fbx=G3D.Viewer.10
assoc .icn=G3D.Viewer.10
assoc .gtm=G3D.Viewer.10
assoc .bsp=G3D.Viewer.10
assoc .md2=G3D.Viewer.10
assoc .md3=G3D.Viewer.10

ftype G3D.Viewer.10=%program% "%%1"

pause

