import csv
from collections import defaultdict

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
stats = calculate_photons_per_task_clock('stats-atom-new.csv')
stats_pc_opt = calculate_photons_per_task_clock('stats-atom.csv')

# Variables para calcular el mayor aumento y promedios
max_improvement = 0
max_improvement_key = None
improvement_by_compiler = defaultdict(list)

# Comparar resultados
for key in stats:
    if key in stats_pc_opt:
        stats_value = stats[key]
        stats_pc_opt_value = stats_pc_opt[key]
        compiler = key[0]  # Extraer el compilador del key

        # Determinar cuál es mejor y calcular el factor de mejora
        if stats_value > stats_pc_opt_value:
            better = "vectorizada"
            improvement = stats_value / stats_pc_opt_value
        else:
            better = "normal"
            improvement = stats_pc_opt_value / stats_value

        # Guardar el improvement por compilador
        improvement_by_compiler[compiler].append(improvement)

        # Actualizar el mayor aumento
        if improvement > max_improvement:
            max_improvement = improvement
            max_improvement_key = key

        # Imprimir resultados
        print(f"{key}: vectorizada={stats_value:.2f}, normal={stats_pc_opt_value:.2f} -> {better}, improvement={improvement:.2f}x")

# Calcular promedios por compilador
print("\nPromedio de aumento por compilador:")
for compiler, improvements in improvement_by_compiler.items():
    avg_improvement = sum(improvements) / len(improvements)
    print(f"{compiler}: {avg_improvement:.2f}x")

# Imprimir el mayor aumento
print("\nMayor aumento de improvement:")
print(f"{max_improvement_key}: improvement = {max_improvement:.2f}x")