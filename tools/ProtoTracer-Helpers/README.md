# ProtoTracer Helper Programs

This repository provides Python scripts that convert or create **ProtoTracer-compatible** `.h` (C++ header) files from various sources. Each tool serves a distinct purpose—turning CSV, FBX, OBJ, GIF, or image files into data arrays, 3D geometry, or material information that ProtoTracer can understand.

Below is an overview of each script:

## Tools Overview

1. **Camera-Creator**  
   - **Input**: `.csv` (list of pixel coordinates or other camera data)  
   - **Output**: Generates a C++ array of Vector2D for each pixel location, which can be included in ProtoTracer.  
   - **Example**: 
     ```bash
     python Camera-Creator.py input.csv output.h
     ```

2. **FBX-Converter**  
   - **Input**: `.fbx` (Autodesk FBX format)  
   - **Output**: ProtoTracer 3D file (.h) supporting blendshapes (morph targets).  
   - **Example**: 
     ```bash
     python FBX-Converter.py model.fbx ProtoTracerModel.h
     ```

3. **OBJ-Converter**  
   - **Input**: `.obj` (Wavefront OBJ format)  
   - **Output**: ProtoTracer 3D file (.h) that supports materials (textures, colors), but **no blendshapes**.  
   - **Example**: 
     ```bash
     python OBJ-Converter.py model.obj ProtoTracerModel.h
     ```

4. **GIF-Converter**  
   - **Input**: `.gif` (Animated GIF)  
   - **Output**: A ProtoTracer animation header (.h) representing animated materials for frames of the GIF.  
   - **Example**: 
     ```bash
     python GIF-Converter.py animation.gif AnimatedMaterial.h
     ```

5. **Image-Converter**  
   - **Input**: Common image formats (`.png`, `.jpg`, `.bmp`, etc.)  
   - **Output**: A ProtoTracer `.h` file with static material data for the image.  
   - **Example**: 
     ```bash
     python Image-Converter.py texture.png StaticMaterial.h
     ```

## Usage

1. **Clone or download** this repository.  
2. **Install Python 3** if you haven’t already.  
3. Navigate to the folder of the specific converter you need.  
4. **Run** the script in your terminal or command prompt (examples above).  
5. **Include** the generated `.h` file in your ProtoTracer C++ project to access the geometry, material, or camera data.

## Contributing

- **Pull requests** are welcome if you’d like to expand or refine the conversion logic.
  - Fork the repository on GitHub.
  - Commit your changes with a descriptive message (git commit -m 'Add YourFeature').
  - Push the branch (git push origin main).
  - Submit a pull request on GitHub.
- **Issues** can be opened for bug reports or feature requests.

## License

ProtoTracer is licensed under the AGPL-3.0. This ensures modifications and contributions benefit the community. If you use or modify this software for a product, you must make the modified code publicly available as per the license.
