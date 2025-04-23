import csv

def calculate_photons_per_task_clock(file_path):
    results = {}
    with open(file_path, 'r') as file:
        reader = csv.DictReader(file)
        for row in reader:
            photons = int(row['PHOTONS'])
            task_clock = float(row['Task-clock'])
            key = (row['Compiler'], row['Flags'], photons)
            results[key] = photons / task_clock
    return results

# Cargar datos de ambos archivos
stats = calculate_photons_per_task_clock('stats-atom-simd.csv')
stats_pc_opt = calculate_photons_per_task_clock('stats-atom.csv')

# Comparar resultados
for key in stats:
    if key in stats_pc_opt:
        stats_value = stats[key]
        stats_pc_opt_value = stats_pc_opt[key]
        better = "vectorizada" if stats_value > stats_pc_opt_value else "normal"
        improvement = stats_value / stats_pc_opt_value  # Calcular el aumento relativo
        print(f"{key}: vectorizada={stats_value:.2f}, normal={stats_pc_opt_value:.2f} -> {better}, improvement={improvement:.2f}x")