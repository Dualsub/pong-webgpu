from pyassimp import load, postprocess
import numpy as np
import argparse
import struct


def main():
    parser = argparse.ArgumentParser(description='Convert model to dat file')
    parser.add_argument('--model', type=str, help='model path')
    parser.add_argument('--output', type=str, help='output dat path')
    parser.add_argument('--scale', type=float,
                        default=1.0, help='scale factor')
    parser.add_argument('--center', type=bool, default=False,
                        help='center model around origin')
    args = parser.parse_args()

    model_path = args.model
    output_path = args.output

    print('Loading model...')
    flags = postprocess.aiProcess_Triangulate | postprocess.aiProcess_JoinIdenticalVertices | postprocess.aiProcess_GenSmoothNormals | postprocess.aiProcess_CalcTangentSpace | postprocess.aiProcess_PreTransformVertices

    with load(model_path, processing=flags) as scene:
        indices = []
        positions = []
        colors = []
        for mesh in scene.meshes:
            # Vertex positions
            max_index = len(positions)
            positions.extend(mesh.vertices)
            color = mesh.material.properties['diffuse']
            colors.extend([color] * len(mesh.vertices))

            new_indices = np.array(
                [face for face in mesh.faces]).flatten().astype(np.uint32) + max_index

            indices.extend(new_indices)

        positions = np.array(positions).astype(np.float32)
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
        vertices = np.hstack([positions, colors])
        assert len(vertices[0]) == 6
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
