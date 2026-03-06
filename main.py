import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import threading
import queue
import os
import sys
import ast
import math
from datetime import datetime, timedelta
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from typing import List, Dict, Tuple, Optional, Callable

# Константы для путей к файлам
ZONE_FILE_PATH: str = "src/zone.txt"
ROLES_FILE_PATH: str = "roles.txt"
COORDINATES_FILE_PATH: str = "src/coordinates.txt"
OUTPUT_FILE_PATH: str = "zone_analysis.txt"

# Определение ролей для зон
ROLE_OPTIONS: Dict[str, str] = {
    "1": "сон",
    "2": "работа",
    "3": "отдых"
}

class Zone:
    """
    Класс для представления зоны с координатами, именем и ролью.

    Атрибуты:
        id (int): Идентификатор зоны.
        coordinates (List[Tuple[int, int]]): Список координат, определяющих зону.
        name (str): Название зоны.
        role (str): Роль зоны (сон, работа, отдых).
    """

    def __init__(self, zone_id: int, coordinates: List[Tuple[int, int]], name: str, role: str) -> None:
        self.id = zone_id
        self.coordinates = coordinates
        self.name = name
        self.role = role

    def __repr__(self) -> str:
        return f"Zone(id={self.id}, name='{self.name}', role='{self.role}', coordinates={self.coordinates})"

class ZoneReader:
    """
    Класс для чтения зон из файла.
    """

    @staticmethod
    def read_zones_from_file(filepath: str) -> List[List[Tuple[int, int]]]:
        """
        Считывает зоны из файла.

        Args:
            filepath (str): Путь к файлу с зонами.

        Returns:
            List[List[Tuple[int, int]]]: Список зон, где каждая зона — это список координат.
        """
        zones: List[List[Tuple[int, int]]] = []
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                for line_num, line in enumerate(f, 1):
                    line = line.strip()
                    if not line:
                        continue
                    coord_pairs_str = line.split(';')
                    if len(coord_pairs_str) < 3:
                        continue
                    current_zone_points: List[Tuple[int, int]] = []
                    valid_zone: bool = True
                    for pair_str in coord_pairs_str:
                        try:
                            x_str, y_str = pair_str.split(',')
                            x = int(x_str.strip())
                            y = int(y_str.strip())
                            current_zone_points.append((x, y))
                        except ValueError:
                            valid_zone = False
                            break
                    if valid_zone:
                        zones.append(current_zone_points)
        except FileNotFoundError:
            print(f"Ошибка: Файл '{filepath}' не найден.")
        except Exception as e:
            print(f"Произошла ошибка при чтении файла '{filepath}': {e}")
        return zones

class ZoneVisualizer:
    """
    Класс для визуализации зон на графике.
    """

    @staticmethod
    def display_zones(zones_data: List[List[Tuple[int, int]]], zone_details: Optional[List[Dict]] = None) -> None:
        """
        Отображает зоны на графике.

        Args:
            zones_data (List[List[Tuple[int, int]]]): Список зон.
            zone_details (Optional[List[Dict]], optional): Детали зон для отображения меток.
        """
        if not zones_data:
            return
        fig, ax = plt.subplots()
        ax.set_aspect('equal', adjustable='datalim')
        all_x_coords: List[int] = []
        all_y_coords: List[int] = []
        for i, zone_points in enumerate(zones_data):
            polygon = patches.Polygon(
                zone_points,
                closed=True,
                edgecolor='blue',
                facecolor='lightblue',
                alpha=0.7
            )
            ax.add_patch(polygon)
            for x, y in zone_points:
                all_x_coords.append(x)
                all_y_coords.append(y)
            label_text = f"Зона {i+1}"
            if zone_points:
                cx = sum(p[0] for p in zone_points) / len(zone_points)
                cy = sum(p[1] for p in zone_points) / len(zone_points)
                ax.text(cx, cy, label_text, ha='center', va='center', color='black', fontsize=8)
        if not all_x_coords or not all_y_coords:
            print("Не удалось определить границы для графика.")
            return
        padding = 10
        ax.set_xlim(min(all_x_coords) - padding, max(all_x_coords) + padding)
        ax.set_ylim(min(all_y_coords) - padding, max(all_y_coords) + padding)
        ax.set_xlabel("Ось X")
        ax.set_ylabel("Ось Y")
        ax.set_title("Определённые зоны")
        ax.grid(True)
        print("Окно с зонами будет отображено. Закройте его, чтобы продолжить.")
        plt.show()

class ZoneDetailsManager:
    """
    Класс для управления деталями зон: чтение, сохранение, ввод пользователем.
    """

    @staticmethod
    def get_zone_details_from_user(
        zones_data: List[List[Tuple[int, int]]],
        gui_input: Callable[[], str],
        gui_output: Callable[[str], None]
    ) -> List[Dict]:
        """
        Получает детали зон от пользователя.

        Args:
            zones_data (List[List[Tuple[int, int]]]): Список зон.
            gui_input (Callable[[], str]): Функция для получения ввода от пользователя.
            gui_output (Callable[[str], None]): Функция для вывода сообщений пользователю.

        Returns:
            List[Dict]: Список словарей с деталями зон.
        """
        if not zones_data:
            return []
        assigned_details_data: List[Dict] = []
        gui_output("\n--- Ввод данных для зон (имя и роль) ---\n")
        for i, zone_points in enumerate(zones_data):
            zone_id = i + 1
            gui_output(f"\nОбработка Зоны {zone_id} (координаты: {zone_points})\n")
            zone_name = ""
            while True:
                gui_output(f"Введите имя для Зоны {zone_id}: ")
                zone_name = gui_input()
                if zone_name:
                    break
                else:
                    gui_output("Имя зоны не может быть пустым. Пожалуйста, введите имя.\n")
            prompt_message = f"Выберите роль для Зоны {zone_id} (Имя: '{zone_name}'):\n"
            for key, value in ROLE_OPTIONS.items():
                prompt_message += f"  {key}: {value}\n"
            prompt_message += "Ваш выбор (1/2/3): "
            while True:
                gui_output(prompt_message)
                choice = gui_input()
                if choice in ROLE_OPTIONS:
                    role_name = ROLE_OPTIONS[choice]
                    assigned_details_data.append({
                        "id": zone_id,
                        "coordinates": zone_points,
                        "name": zone_name,
                        "role": role_name
                    })
                    gui_output(f"Для Зоны {zone_id} (Имя: '{zone_name}') назначена роль: '{role_name}'.\n")
                    break
                else:
                    gui_output("Неверный ввод. Пожалуйста, выберите 1, 2 или 3.\n")
        return assigned_details_data

    @staticmethod
    def save_details_to_file(assigned_details: List[Dict], filepath: str) -> None:
        """
        Сохраняет детали зон в файл.

        Args:
            assigned_details (List[Dict]): Список словарей с деталями зон.
            filepath (str): Путь к файлу для сохранения.
        """
        try:
            with open(filepath, 'w', encoding='utf-8') as f:
                if not assigned_details:
                    f.write("")
                    print(f"Файл '{filepath}' очищен/оставлен пустым.")
                    return
                for entry in assigned_details:
                    f.write(f"Zone ID: {entry['id']}\n")
                    f.write(f"Coordinates: {entry['coordinates']}\n")
                    f.write(f"Name: {entry['name']}\n")
                    f.write(f"Role: {entry['role']}\n")
                    f.write("---\n")
            print(f"\nДанные о зонах успешно сохранены в файл '{filepath}'.")
        except Exception as e:
            print(f"Ошибка при сохранении файла '{filepath}': {e}")

    @staticmethod
    def read_previous_zone_details(filepath: str) -> List[Dict]:
        """
        Считывает предыдущие детали зон из файла.

        Args:
            filepath (str): Путь к файлу с деталями зон.

        Returns:
            List[Dict]: Список словарей с деталями зон.
        """
        previous_details: List[Dict] = []
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                content = f.read().split('---\n')
                for block in content:
                    block = block.strip()
                    if not block:
                        continue
                    detail_data: Dict = {}
                    lines = block.split('\n')
                    if len(lines) >= 4:
                        try:
                            if lines[0].startswith("Zone ID: "):
                                detail_data['id'] = int(lines[0].replace("Zone ID: ", "").strip())
                            if lines[1].startswith("Coordinates: "):
                                coords_str = lines[1].replace("Coordinates: ", "").strip()
                                detail_data['coordinates'] = ast.literal_eval(coords_str)
                            if lines[2].startswith("Name: "):
                                detail_data['name'] = lines[2].replace("Name: ", "").strip()
                            if lines[3].startswith("Role: "):
                                detail_data['role'] = lines[3].replace("Role: ", "").strip()
                            if all(key in detail_data for key in ['id', 'coordinates', 'name', 'role']):
                                previous_details.append(detail_data)
                            else:
                                print(f"Предупреждение: Не удалось полностью разобрать блок в '{filepath}':\n{block}")
                        except (ValueError, SyntaxError) as e:
                            print(f"Ошибка парсинга блока в '{filepath}': {e}\nБлок:\n{block}")
        except FileNotFoundError:
            print(f"Инфо: Файл '{filepath}' не найден (вероятно, первый запуск).")
        except Exception as e:
            print(f"Ошибка при чтении файла '{filepath}': {e}")
        previous_details.sort(key=lambda x: x.get('id', float('inf')))
        return previous_details

class CoordinateReader:
    """
    Класс для чтения координат из файла.
    """

    @staticmethod
    def read_coordinates_from_file(filepath: str) -> List[Dict]:
        """
        Считывает координаты из файла.

        Args:
            filepath (str): Путь к файлу с координатами.

        Returns:
            List[Dict]: Список словарей с координатами и временем.
        """
        coordinates_data: List[Dict] = []
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                for line_num, line in enumerate(f, 1):
                    line = line.strip()
                    if not line:
                        continue
                    try:
                        parts = line.split(',')
                        if len(parts) != 3:
                            print(f"Предупреждение: Строка {line_num} имеет неверный формат: '{line}'")
                            continue
                        x = int(parts[0].strip())
                        y = int(parts[1].strip())
                        time_str = parts[2].strip()
                        time_parts = time_str.split(':')
                        if len(time_parts) != 6:
                            print(f"Предупреждение: Неверный формат времени в строке {line_num}: '{time_str}'")
                            continue
                        year, month, day, hour, minute, second = map(int, time_parts)
                        datetime_obj = datetime(year, month, day, hour, minute, second)
                        coordinates_data.append({
                            'x': x,
                            'y': y,
                            'timestamp': time_str,
                            'datetime_obj': datetime_obj
                        })
                    except ValueError as e:
                        print(f"Ошибка парсинга строки {line_num}: '{line}' - {e}")
                        continue
        except FileNotFoundError:
            print(f"Ошибка: Файл '{filepath}' не найден.")
        except Exception as e:
            print(f"Ошибка при чтении файла '{filepath}': {e}")
        coordinates_data.sort(key=lambda x: x['datetime_obj'])
        return coordinates_data

class ZoneAnalyzer:
    """
    Класс для анализа пребывания в зонах.
    """

    @staticmethod
    def point_in_polygon(x: int, y: int, polygon_coords: List[Tuple[int, int]]) -> bool:
        """
        Проверяет, находится ли точка внутри многоугольника.

        Args:
            x (int): Координата X точки.
            y (int): Координата Y точки.
            polygon_coords (List[Tuple[int, int]]): Координаты вершин многоугольника.

        Returns:
            bool: True, если точка внутри многоугольника, иначе False.
        """
        n = len(polygon_coords)
        inside = False
        p1x, p1y = polygon_coords[0]
        for i in range(1, n + 1):
            p2x, p2y = polygon_coords[i % n]
            if y > min(p1y, p2y):
                if y <= max(p1y, p2y):
                    if x <= max(p1x, p2x):
                        if p1y != p2y:
                            xinters = (y - p1y) * (p2x - p1x) / (p2y - p1y) + p1x
                        if p1x == p2x or x <= xinters:
                            inside = not inside
            p1x, p1y = p2x, p2y
        return inside

    @staticmethod
    def analyze_zone_presence(
        coordinates_data: List[Dict],
        zone_details: List[Dict]
    ) -> Dict:
        """
        Анализирует пребывание в зонах и возвращает результаты анализа.

        Args:
            coordinates_data (List[Dict]): Список координат с временем.
            zone_details (List[Dict]): Детали зон.

        Returns:
            Dict: Результаты анализа по зонам.
        """
        zone_analysis: Dict = {}
        for zone in zone_details:
            zone_id = zone['id']
            zone_name = zone['name']
            zone_role = zone['role']
            zone_coords = zone['coordinates']
            points_in_zone = []
            for coord in coordinates_data:
                if ZoneAnalyzer.point_in_polygon(coord['x'], coord['y'], zone_coords):
                    points_in_zone.append(coord)
            if not points_in_zone:
                zone_analysis[zone_id] = {
                    'name': zone_name,
                    'role': zone_role,
                    'coordinates': zone_coords,
                    'visits': [],
                    'total_time': timedelta(0),
                    'visit_count': 0
                }
                continue
            visits = []
            current_visit = [points_in_zone[0]]
            for i in range(1, len(points_in_zone)):
                current_point = points_in_zone[i]
                previous_point = points_in_zone[i-1]
                time_diff = current_point['datetime_obj'] - previous_point['datetime_obj']
                if time_diff > timedelta(minutes=30):
                    visits.append(current_visit)
                    current_visit = [current_point]
                else:
                    current_visit.append(current_point)
            visits.append(current_visit)
            visit_periods = []
            total_time_in_zone = timedelta(0)
            for visit_points_list in visits:
                duration = timedelta(0)
                start_time = visit_points_list[0]['datetime_obj']
                end_time = visit_points_list[-1]['datetime_obj']
                if len(visit_points_list) == 1:
                    duration = timedelta(minutes=5)
                    end_time = start_time + duration
                else:
                    duration = end_time - start_time
                    if duration <= timedelta(0):
                        duration = timedelta(minutes=1)
                visit_periods.append({
                    'start': start_time,
                    'end': end_time,
                    'duration': duration,
                    'points_count': len(visit_points_list),
                    'date': start_time.date()
                })
                total_time_in_zone += duration
            zone_analysis[zone_id] = {
                'name': zone_name,
                'role': zone_role,
                'coordinates': zone_coords,
                'visits': visit_periods,
                'total_time': total_time_in_zone,
                'visit_count': len(visit_periods)
            }
        return zone_analysis

    @staticmethod
    def time_to_seconds_from_midnight(t: datetime.time) -> float:
        """
        Преобразует время в секунды с начала дня.

        Args:
            t (datetime.time): Время.

        Returns:
            float: Количество секунд с начала дня.
        """
        return t.hour * 3600 + t.minute * 60 + t.second

    @staticmethod
    def seconds_from_midnight_to_time_str(s: float) -> str:
        """
        Преобразует секунды с начала дня в строку времени.

        Args:
            s (float): Количество секунд с начала дня.

        Returns:
            str: Строка времени в формате "HH:MM:SS".
        """
        if s is None or math.isnan(s) or s < 0:
            return "N/A"
        s = round(s)
        hours = int(s // 3600) % 24
        minutes = int((s % 3600) // 60)
        seconds = int(s % 60)
        return f"{hours:02d}:{minutes:02d}:{seconds:02d}"

    @staticmethod
    def format_timedelta_hhmmss(td: timedelta) -> str:
        """
        Форматирует timedelta в строку "HH:MM:SS".

        Args:
            td (timedelta): Временной интервал.

        Returns:
            str: Строка в формате "HH:MM:SS".
        """
        if td is None:
            return "N/A"
        total_seconds = round(td.total_seconds())
        if total_seconds < 0:
            return "Invalid timedelta"
        hours, remainder = divmod(total_seconds, 3600)
        minutes, seconds = divmod(remainder, 60)
        return f"{int(hours):02d}:{int(minutes):02d}:{int(seconds):02d}"

    @staticmethod
    def calculate_behavioral_patterns(zone_analysis_data: Dict) -> Dict:
        """
        Рассчитывает поведенческие паттерны (сон, работа).

        Args:
            zone_analysis_data (Dict): Данные анализа зон.

        Returns:
            Dict: Поведенческие паттерны.
        """
        sleep_periods: List[Dict] = []
        work_periods: List[Dict] = []
        for zone_id, data in zone_analysis_data.items():
            role = data.get('role', '').lower()
            for visit in data.get('visits', []):
                if visit['duration'] > timedelta(minutes=5):
                    period_detail = {
                        'zone_name': data.get('name', 'N/A'),
                        'start_datetime': visit['start'],
                        'end_datetime': visit['end'],
                        'duration_timedelta': visit['duration']
                    }
                    if role == 'сон':
                        sleep_periods.append(period_detail)
                    elif role == 'работа':
                        work_periods.append(period_detail)
        sleep_summary = {
            "periods": sorted(sleep_periods, key=lambda x: x['start_datetime']),
            "avg_bedtime": "N/A",
            "avg_waketime": "N/A",
            "avg_duration": "N/A",
            "total_duration": ZoneAnalyzer.format_timedelta_hhmmss(
                sum((p['duration_timedelta'] for p in sleep_periods), timedelta())
            ),
            "count": len(sleep_periods)
        }
        if sleep_periods:
            bedtimes_seconds = [
                ZoneAnalyzer.time_to_seconds_from_midnight(p['start_datetime'].time())
                for p in sleep_periods
            ]
            waketimes_seconds = [
                ZoneAnalyzer.time_to_seconds_from_midnight(p['end_datetime'].time())
                for p in sleep_periods
            ]
            avg_bedtime_s = sum(bedtimes_seconds) / len(bedtimes_seconds)
            avg_waketime_s = sum(waketimes_seconds) / len(waketimes_seconds)
            sleep_summary["avg_bedtime"] = ZoneAnalyzer.seconds_from_midnight_to_time_str(avg_bedtime_s)
            sleep_summary["avg_waketime"] = ZoneAnalyzer.seconds_from_midnight_to_time_str(avg_waketime_s)
            total_sleep_td = sum((p['duration_timedelta'] for p in sleep_periods), timedelta())
            if sleep_periods:
                sleep_summary["avg_duration"] = ZoneAnalyzer.format_timedelta_hhmmss(total_sleep_td / len(sleep_periods))
        work_summary = {
            "periods": sorted(work_periods, key=lambda x: x['start_datetime']),
            "avg_start_time": "N/A",
            "avg_end_time": "N/A",
            "avg_duration_per_session": "N/A",
            "total_duration": ZoneAnalyzer.format_timedelta_hhmmss(
                sum((p['duration_timedelta'] for p in work_periods), timedelta())
            ),
            "count": len(work_periods)
        }
        if work_periods:
            work_start_times_s = [
                ZoneAnalyzer.time_to_seconds_from_midnight(p['start_datetime'].time())
                for p in work_periods
            ]
            work_end_times_s = [
                ZoneAnalyzer.time_to_seconds_from_midnight(p['end_datetime'].time())
                for p in work_periods
            ]
            avg_work_start_s = sum(work_start_times_s) / len(work_start_times_s)
            avg_work_end_s = sum(work_end_times_s) / len(work_end_times_s)
            work_summary["avg_start_time"] = ZoneAnalyzer.seconds_from_midnight_to_time_str(avg_work_start_s)
            work_summary["avg_end_time"] = ZoneAnalyzer.seconds_from_midnight_to_time_str(avg_work_end_s)
            total_work_td = sum((p['duration_timedelta'] for p in work_periods), timedelta())
            if work_periods:
                work_summary["avg_duration_per_session"] = ZoneAnalyzer.format_timedelta_hhmmss(
                    total_work_td / len(work_periods)
                )
        return {"sleep": sleep_summary, "work": work_summary}

class ReportGenerator:
    """
    Класс для генерации отчётов.
    """

    @staticmethod
    def save_analysis_to_file(
        zone_analysis: Dict,
        behavioral_patterns: Dict,
        filepath: str
    ) -> None:
        """
        Сохраняет анализ в файл.

        Args:
            zone_analysis (Dict): Анализ зон.
            behavioral_patterns (Dict): Поведенческие паттерны.
            filepath (str): Путь к файлу для сохранения.
        """
        try:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write("=" * 60 + "\n")
                f.write("АНАЛИЗ ВРЕМЕНИ ПРЕБЫВАНИЯ В ЗОНАХ\n")
                f.write("=" * 60 + "\n\n")
                if not zone_analysis:
                    f.write("Данные о зонах отсутствуют.\n")
                else:
                    total_zones = len(zone_analysis)
                    zones_with_visits = sum(1 for z in zone_analysis.values() if z['visit_count'] > 0)
                    total_visits_all_zones = sum(z['visit_count'] for z in zone_analysis.values())
                    f.write(f"ОБЩАЯ СТАТИСТИКА ПО ЗОНАМ:\n")
                    f.write(f"- Всего зон: {total_zones}\n")
                    f.write(f"- Зон с посещениями: {zones_with_visits}\n")
                    f.write(f"- Общее количество посещений (все зоны): {total_visits_all_zones}\n\n")
                    for zone_id in sorted(zone_analysis.keys()):
                        zone_data = zone_analysis[zone_id]
                        f.write("-" * 50 + "\n")
                        f.write(f"ЗОНА {zone_id}: {zone_data['name']}\n")
                        f.write(f"Роль: {zone_data['role']}\n")
                        f.write(f"Координаты: {zone_data['coordinates']}\n")
                        f.write("-" * 50 + "\n")
                        if zone_data['visit_count'] == 0:
                            f.write("Посещений не было.\n\n")
                            continue
                        f.write(f"Количество посещений: {zone_data['visit_count']}\n")
                        f.write(f"Общее время пребывания: {ZoneAnalyzer.format_timedelta_hhmmss(zone_data['total_time'])}\n\n")
                        f.write("ПЕРИОДЫ ПОСЕЩЕНИЙ:\n")
                        for i, visit in enumerate(zone_data['visits'], 1):
                            f.write(f"  {i}. {visit['start'].strftime('%Y-%m-%d %H:%M')} - "
                                    f"{visit['end'].strftime('%Y-%m-%d %H:%M')} "
                                    f"(Длительность: {ZoneAnalyzer.format_timedelta_hhmmss(visit['duration'])}, "
                                    f"Точек: {visit['points_count']})\n")
                        f.write("\n")
                # Анализ сна
                sleep_summary = behavioral_patterns.get("sleep", {})
                f.write("\n" + "=" * 60 + "\n")
                f.write("АНАЛИЗ СНА\n")
                f.write("=" * 60 + "\n")
                if not sleep_summary or sleep_summary.get("count", 0) == 0:
                    f.write("Нет данных для анализа сна или периоды сна отсутствуют/слишком короткие.\n")
                else:
                    f.write(f"Всего периодов сна (>5 мин): {sleep_summary['count']}\n")
                    f.write(f"Среднее время отхода ко сну: {sleep_summary['avg_bedtime']}\n")
                    f.write(f"Среднее время пробуждения: {sleep_summary['avg_waketime']}\n")
                    f.write(f"Средняя продолжительность сна: {sleep_summary['avg_duration']}\n")
                    f.write(f"Общая продолжительность сна: {sleep_summary['total_duration']}\n\n")
                    f.write("ДЕТАЛИЗАЦИЯ ПЕРИОДОВ СНА:\n")
                    for i, period in enumerate(sleep_summary.get("periods", []), 1):
                        f.write(f"  {i}. Зона: '{period['zone_name']}', "
                                f"{period['start_datetime'].strftime('%Y-%m-%d %H:%M')} - "
                                f"{period['end_datetime'].strftime('%Y-%m-%d %H:%M')} "
                                f"(Длительность: {ZoneAnalyzer.format_timedelta_hhmmss(period['duration_timedelta'])})\n")
                f.write("\n")
                # Анализ работы
                work_summary = behavioral_patterns.get("work", {})
                f.write("\n" + "=" * 60 + "\n")
                f.write("АНАЛИЗ РАБОТЫ\n")
                f.write("=" * 60 + "\n")
                if not work_summary or work_summary.get("count", 0) == 0:
                    f.write("Нет данных для анализа работы или рабочие сессии отсутствуют/слишком короткие.\n")
                else:
                    f.write(f"Всего рабочих сессий (>5 мин): {work_summary['count']}\n")
                    f.write(f"Среднее время начала работы: {work_summary['avg_start_time']}\n")
                    f.write(f"Среднее время окончания работы: {work_summary['avg_end_time']}\n")
                    f.write(f"Средняя продолжительность рабочей сессии: {work_summary['avg_duration_per_session']}\n")
                    f.write(f"Общая продолжительность работы: {work_summary['total_duration']}\n\n")
                    f.write("ДЕТАЛИЗАЦИЯ РАБОЧИХ СЕССИЙ:\n")
                    for i, period in enumerate(work_summary.get("periods", []), 1):
                        f.write(f"  {i}. Зона: '{period['zone_name']}', "
                                f"{period['start_datetime'].strftime('%Y-%m-%d %H:%M')} - "
                                f"{period['end_datetime'].strftime('%Y-%m-%d %H:%M')} "
                                f"(Длительность: {ZoneAnalyzer.format_timedelta_hhmmss(period['duration_timedelta'])})\n")
            print(f"Результаты анализа сохранены в файл '{filepath}'")
        except Exception as e:
            print(f"Ошибка при сохранении файла '{filepath}': {e}")

class MovementVisualizer:
    """
    Класс для визуализации зон и траектории движения.
    """

    @staticmethod
    def visualize_zones_and_movement(
        zone_details: List[Dict],
        coordinates_data: List[Dict],
        zone_analysis: Dict
    ) -> None:
        """
        Визуализирует зоны и траекторию движения.

        Args:
            zone_details (List[Dict]): Детали зон.
            coordinates_data (List[Dict]): Координаты движения.
            zone_analysis (Dict): Анализ зон.
        """
        if not zone_details and not coordinates_data:
            print("Нет данных для визуализации.")
            return
        fig, ax = plt.subplots(figsize=(12, 8))
        ax.set_aspect('equal', adjustable='datalim')
        all_x_coords: List[int] = []
        all_y_coords: List[int] = []
        colors = ['lightblue', 'lightgreen', 'lightcoral', 'lightyellow', 'lightpink', 'cyan', 'magenta']
        for i, zone in enumerate(zone_details):
            zone_coords = zone['coordinates']
            color = colors[i % len(colors)]
            polygon = patches.Polygon(
                zone_coords,
                closed=True,
                edgecolor='black',
                facecolor=color,
                alpha=0.6
            )
            ax.add_patch(polygon)
            for x, y in zone_coords:
                all_x_coords.append(x)
                all_y_coords.append(y)
            if zone_coords:
                cx = sum(p[0] for p in zone_coords) / len(zone_coords)
                cy = sum(p[1] for p in zone_coords) / len(zone_coords)
                zone_info_from_analysis = zone_analysis.get(zone['id'], {})
                visit_count = zone_info_from_analysis.get('visit_count', 0)
                total_time_in_zone_td = zone_info_from_analysis.get('total_time', timedelta(0))
                label_text = (
                    f"{zone['name']}\n({zone['role']})\n"
                    f"Посещений: {visit_count}\nВремя: {ZoneAnalyzer.format_timedelta_hhmmss(total_time_in_zone_td)}"
                )
                ax.text(
                    cx, cy, label_text,
                    ha='center', va='center',
                    color='black', fontsize=7,
                    bbox=dict(boxstyle="round,pad=0.3", facecolor='white', alpha=0.7)
                )
        if coordinates_data:
            x_coords_traj = [coord['x'] for coord in coordinates_data]
            y_coords_traj = [coord['y'] for coord in coordinates_data]
            all_x_coords.extend(x_coords_traj)
            all_y_coords.extend(y_coords_traj)
            ax.plot(
                x_coords_traj, y_coords_traj,
                'r-', alpha=0.5, linewidth=1, label='Траектория движения'
            )
        if all_x_coords and all_y_coords:
            padding = 20
            ax.set_xlim(min(all_x_coords) - padding, max(all_x_coords) + padding)
            ax.set_ylim(min(all_y_coords) - padding, max(all_y_coords) + padding)
        ax.set_xlabel("Ось X")
        ax.set_ylabel("Ось Y")
        ax.set_title("Зоны и траектория движения")
        ax.grid(True, alpha=0.3)
        ax.legend()
        print("Отображение графика с зонами и траекторией движения...")
        plt.show()

class ConsoleRedirect:
    """
    Класс для перенаправления вывода консоли в виджет Tkinter.
    """

    def __init__(self, text_widget: scrolledtext.ScrolledText, queue_obj: queue.Queue) -> None:
        self.text_widget = text_widget
        self.queue = queue_obj

    def write(self, string: str) -> None:
        self.queue.put(string)

    def flush(self) -> None:
        pass

class ZoneAnalysisGUI:
    """
    Основной класс графического интерфейса для анализа зон.
    """

    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("Анализ зон - Система отслеживания")
        self.root.geometry("900x670")
        self.console_queue = queue.Queue()
        self.is_running = False
        self.create_widgets()
        self.process_console_queue()
        self.console_redirect = ConsoleRedirect(self.console_text, self.console_queue)
        self.old_stdout = sys.stdout
        self.old_stderr = sys.stderr

    def create_widgets(self) -> None:
        """Создаёт виджеты интерфейса."""
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(1, weight=1)

        title_label = ttk.Label(
            main_frame,
            text="Система анализа зон",
            font=('Arial', 16, 'bold')
        )
        title_label.grid(row=0, column=0, pady=(0, 10), sticky=tk.W)

        # Консоль
        console_frame = ttk.LabelFrame(main_frame, text="Вывод консоли", padding="5")
        console_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=(0, 10))
        console_frame.columnconfigure(0, weight=1)
        console_frame.rowconfigure(0, weight=1)

        self.console_text = scrolledtext.ScrolledText(
            console_frame,
            wrap=tk.WORD,
            width=100,
            height=28,
            font=('Consolas', 9),
            state=tk.DISABLED
        )
        self.console_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # Кнопки управления
        control_frame = ttk.Frame(main_frame)
        control_frame.grid(row=2, column=0, sticky=(tk.W, tk.E))

        self.btn_zones = ttk.Button(
            control_frame,
            text="Ввод зон, ролей и имён",
            command=self.start_zones
        )
        self.btn_zones.grid(row=0, column=0, padx=(0, 10))

        self.btn_analysis = ttk.Button(
            control_frame,
            text="Анализ по координатам",
            command=self.start_analysis
        )
        self.btn_analysis.grid(row=0, column=1, padx=(0, 10))

        self.btn_visual = ttk.Button(
            control_frame,
            text="Визуализация зон и движения",
            command=self.visualize
        )
        self.btn_visual.grid(row=0, column=2, padx=(0, 10))

        self.btn_found_zones = ttk.Button(
            control_frame,
            text="Найденные зоны",
            command=self.show_found_zones
        )
        self.btn_found_zones.grid(row=0, column=3, padx=(0, 10))

        self.btn_clear = ttk.Button(
            control_frame,
            text="Очистить консоль",
            command=self.clear_console
        )
        self.btn_clear.grid(row=0, column=4, padx=(0, 10))

        self.progress = ttk.Progressbar(control_frame, mode='indeterminate')
        self.progress.grid(row=0, column=5, sticky=(tk.W, tk.E), padx=(10, 0))
        control_frame.columnconfigure(5, weight=1)

    def write_to_console(self, text: str, tag: Optional[str] = None) -> None:
        """Записывает текст в консоль."""
        self.console_text.config(state=tk.NORMAL)
        self.console_text.insert(tk.END, text)
        self.console_text.see(tk.END)
        self.console_text.config(state=tk.DISABLED)

    def process_console_queue(self) -> None:
        """Обрабатывает очередь сообщений для консоли."""
        try:
            while True:
                message = self.console_queue.get_nowait()
                self.write_to_console(message)
        except queue.Empty:
            pass
        self.root.after(100, self.process_console_queue)

    def clear_console(self) -> None:
        """Очищает консоль."""
        self.console_text.config(state=tk.NORMAL)
        self.console_text.delete(1.0, tk.END)
        self.console_text.config(state=tk.DISABLED)

    def show_found_zones(self) -> None:
        """Показывает найденные зоны из файла."""
        def task() -> None:
            sys.stdout = self.console_redirect
            sys.stderr = self.console_redirect
            self.progress.start()
            zones = ZoneReader.read_zones_from_file(ZONE_FILE_PATH)
            if zones:
                self.gui_output(f"Обнаружено зон: {len(zones)}\n")
                ZoneVisualizer.display_zones(zones)
            else:
                self.gui_output("Зоны не найдены (src/zone.txt пуст или отсутствует)\n")
            self.progress.stop()
            sys.stdout = self.old_stdout
            sys.stderr = self.old_stderr
        threading.Thread(target=task, daemon=True).start()

    def gui_input(self) -> str:
        """Запрашивает строку через диалоговое окно ввода."""
        popup = tk.Toplevel(self.root)
        popup.title("Ввод")
        popup.geometry("350x120")
        label = ttk.Label(popup, text="Введите значение:")
        label.pack(pady=7)
        entry_var = tk.StringVar()
        entry = ttk.Entry(popup, textvariable=entry_var, width=32)
        entry.pack(pady=5)
        entry.focus_set()
        result: List[str] = []
        def on_ok() -> None:
            result.append(entry_var.get())
            popup.destroy()
        def on_cancel() -> None:
            result.append("")
            popup.destroy()
        btn_ok = ttk.Button(popup, text="OK", command=on_ok)
        btn_ok.pack(side=tk.LEFT, padx=30, pady=5, expand=True)
        btn_cancel = ttk.Button(popup, text="Отмена", command=on_cancel)
        btn_cancel.pack(side=tk.RIGHT, padx=30, pady=5, expand=True)
        popup.grab_set()
        self.root.wait_window(popup)
        return result[0] if result else ""

    def gui_output(self, s: str) -> None:
        """Выводит строку в консоль GUI."""
        self.console_queue.put(str(s))

    def start_zones(self) -> None:
        """Запускает процесс ввода зон, ролей и имён."""
        def task() -> None:
            sys.stdout = self.console_redirect
            sys.stderr = self.console_redirect
            self.progress.start()
            zones = ZoneReader.read_zones_from_file(ZONE_FILE_PATH)
            previous = ZoneDetailsManager.read_previous_zone_details(ROLES_FILE_PATH)
            if not zones:
                self.gui_output("Нет зон для ввода данных.\n")
                self.progress.stop()
                sys.stdout = self.old_stdout
                sys.stderr = self.old_stderr
                return
            details = ZoneDetailsManager.get_zone_details_from_user(
                zones,
                gui_input=self.gui_input,
                gui_output=self.gui_output
            )
            ZoneDetailsManager.save_details_to_file(details, ROLES_FILE_PATH)
            self.progress.stop()
            sys.stdout = self.old_stdout
            sys.stderr = self.old_stderr
        threading.Thread(target=task, daemon=True).start()

    def start_analysis(self) -> None:
        """Запускает процесс анализа по координатам."""
        def task() -> None:
            sys.stdout = self.console_redirect
            sys.stderr = self.console_redirect
            self.progress.start()
            coordinates_data = CoordinateReader.read_coordinates_from_file(COORDINATES_FILE_PATH)
            zone_details = ZoneDetailsManager.read_previous_zone_details(ROLES_FILE_PATH)
            if not coordinates_data or not zone_details:
                self.gui_output("Для анализа необходимы координаты и зоны.\n")
                self.progress.stop()
                sys.stdout = self.old_stdout
                sys.stderr = self.old_stderr
                return
            zone_analysis_result = ZoneAnalyzer.analyze_zone_presence(coordinates_data, zone_details)
            behavioral_patterns_result = ZoneAnalyzer.calculate_behavioral_patterns(zone_analysis_result)
            ReportGenerator.save_analysis_to_file(zone_analysis_result, behavioral_patterns_result, OUTPUT_FILE_PATH)
            self.gui_output("\nАнализ завершён, результаты сохранены в файл.\n")
            self.progress.stop()
            sys.stdout = self.old_stdout
            sys.stderr = self.old_stderr
        threading.Thread(target=task, daemon=True).start()

    def visualize(self) -> None:
        """Запускает процесс визуализации зон и траектории движения."""
        def task() -> None:
            sys.stdout = self.console_redirect
            sys.stderr = self.console_redirect
            self.progress.start()
            coordinates_data = CoordinateReader.read_coordinates_from_file(COORDINATES_FILE_PATH)
            zone_details = ZoneDetailsManager.read_previous_zone_details(ROLES_FILE_PATH)
            zone_analysis_result = (
                ZoneAnalyzer.analyze_zone_presence(coordinates_data, zone_details)
                if coordinates_data and zone_details
                else {}
            )
            MovementVisualizer.visualize_zones_and_movement(zone_details, coordinates_data, zone_analysis_result)
            self.progress.stop()
            sys.stdout = self.old_stdout
            sys.stderr = self.old_stderr
        threading.Thread(target=task, daemon=True).start()

def main() -> None:
    """Основная функция для запуска приложения."""
    root = tk.Tk()
    style = ttk.Style()
    try:
        style.theme_use('clam')
    except:
        pass
    app = ZoneAnalysisGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()
