import argparse
import json
import matplotlib.pyplot as plt
import numpy as np

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("filepath")
    args = parser.parse_args()
    filepath = args.filepath

    # read json file
    with open(filepath) as input_file:
        data = json.load(input_file)
        caches = data["context"]["caches"]
        benchmarks = data["benchmarks"]

    # parse data
    family_indices = np.array([int(bm["family_index"]) for bm in benchmarks])
    bm_names = np.array([bm["name"] for bm in benchmarks])
    element_sizes = np.array([int(name.split('<')[1].split(',')[0])
                              for name in bm_names])
    containers = np.array([name.split(', ')[1] for name in bm_names])
    initializers = np.array([name.split(', ')[2].split('>')[0]
                             for name in bm_names])
    collection_sizes = np.array([int(name.split('/')[1]) for name in bm_names])
    collection_items = collection_sizes / element_sizes
    items_per_second = np.array([float(bm["items_per_second"])
                                 for bm in benchmarks])
    seconds_per_item = 1.0 / items_per_second
    seconds_per_item_per_items = seconds_per_item / collection_items

    # Plot data
    unique_element_sizes = np.unique(element_sizes)
    for element_size in unique_element_sizes:
        plt.figure(figsize=(16,12))

        element_size_indices, = np.where(element_sizes == element_size)
        current_family_indices = family_indices[element_size_indices]
        unique_family_indices = np.unique(current_family_indices)
        for family_index in unique_family_indices:
            current_indices, = np.where(family_indices == family_index)
            container = containers[current_indices[0]]
            initializer = initializers[current_indices[0]]
            if container == 'NullContainer':
                continue
            elif container == 'std::vector':
                label = f"std::vector<{element_size}B>"
                color = 'b'
                if initializer == 'Reserver':
                    label += " (reserved)"
                    color = 'g'
            elif container == 'PointerVector':
                label = f"std::vector<{element_size}B*>"
                color = 'r'
                if initializer == 'Reserver':
                    label += " (reserved)"
                    color = 'orange'
            elif container == 'std::list':
                label = f"std::list<{element_size}B>"
                color = 'y'
            else:
                raise ValueError()

            if 'vector' in label:
                marker = 'v'
            else:
                marker = '^'

            plt.loglog(collection_sizes[current_indices],
                       seconds_per_item[current_indices],
                       label=label, marker=marker, color=color)

        l1_cache = [cache for cache in caches
                    if cache['level'] == 1 and cache['type'] == 'Data'][0]
        plt.axvline(l1_cache['size'], color='b', alpha=0.5, linestyle='--',
                    label='Caches')

        l2_cache = [cache for cache in caches if cache['level'] == 2][0]
        plt.axvline(l2_cache['size'], color='b', alpha=0.5, linestyle='--')

        l3_cache = [cache for cache in caches if cache['level'] == 3][0]
        plt.axvline(l3_cache['size'], color='b', alpha=0.5, linestyle='--')

        plt.xlabel("Container Size (B)")
        plt.ylabel("Time per Insert (s)")
        plt.title(f"Performance of Random Insertion with {element_size}B "
                  f"Elements")
        plt.legend()
        plt.grid()
        plt.savefig(f"random_insertion_performance_plot_{element_size}B.jpg")
        plt.show()

if __name__ == "__main__":
    main()
