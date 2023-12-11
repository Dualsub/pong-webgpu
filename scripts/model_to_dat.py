from pyassimp import load, postprocess
import numpy as np
import argparse
import struct
import cv2 as cv


def sample_colors(texture_coords, texture):
    # For each texture coordinate, sample the color from the texture
    colors = []
    for coord in texture_coords:
        # Get the color from the texture
        x = int(coord[0] * texture.shape[1])
        y = int(coord[1] * texture.shape[0])
        color = texture[y, x]
        # color =
        cv.cvtColor(color, cv.COLOR_BGR2RGB)
        color = color.astype(np.float32) / 255.0
        colors.append(color)
    return colors


def main():
    parser = argparse.ArgumentParser(description='Convert model to dat file')
    parser.add_argument('--model', type=str, help='model path')
    parser.add_argument('--output', type=str, help='output dat path')
    parser.add_argument('--scale', type=float,
                        default=1.0, help='scale factor')
    parser.add_argument('--center', type=bool, default=False,
                        help='center model around origin')
    # Argument for a texture to sample color from
    parser.add_argument('--texture', type=str,
                        help='texture path', default=None)

    args = parser.parse_args()

    model_path = args.model
    output_path = args.output

    print('Loading model...')
    flags = postprocess.aiProcess_Triangulate | postprocess.aiProcess_JoinIdenticalVertices | postprocess.aiProcess_GenSmoothNormals | postprocess.aiProcess_CalcTangentSpace | postprocess.aiProcess_PreTransformVertices | postprocess.aiProcess_FlipUVs | postprocess.aiProcess_FlipWindingOrder

    with load(model_path, processing=flags) as scene:
        indices = []
        positions = []
        normals = []
        colors = []

        # Number of meshes
        print("Number of meshes: ", len(scene.meshes))
        if args.texture is not None:
            texture = cv.imread(args.texture)
            print("texture: ", texture.shape)

        for mesh in scene.meshes:
            # Vertex positions
            max_index = len(positions)
            positions.extend(mesh.vertices)
            normals.extend(mesh.normals)
            color = mesh.material.properties['diffuse']
            new_colors = [color] * len(mesh.vertices) if args.texture is None else sample_colors(
                mesh.texturecoords[0], texture)
            colors.extend(new_colors)

            new_indices = np.array(
                [face for face in mesh.faces]).flatten().astype(np.uint32) + max_index

            indices.extend(new_indices)

        positions = np.array(positions).astype(np.float32) * args.scale
        normals = np.array(normals).astype(np.float32)
        colors = np.array(colors).astype(np.float32)
        print("positions: ", positions.shape)
        print("colors: ", colors.shape)
        indices = np.array(indices).astype(np.uint32)

        if args.center:
            # # Find average of all positions
            average = np.average(positions, axis=0)
            print("average: ", average)

            # Subtract average from all positions
            positions -= average

        # Combine positions and colors into a single array
        vertices = np.hstack([positions, normals, colors])
        assert len(vertices[0]) == 9
        # print largest value in positions
        print("max value in positions: ", np.max(positions))

    # Write to a binary file
    with open(output_path, 'wb') as f:
        f.write(struct.pack('I', len(vertices)))
        print("vertices: ", len(vertices))
        f.write(vertices.tobytes())
        f.write(struct.pack('I', len(indices)))
        print("indices: ", len(indices))
        f.write(indices.tobytes())

    print('Done!')


main()
