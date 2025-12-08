# cartographer.py
import tkinter as tk
from tkinter import ttk, filedialog, messagebox, simpledialog
from PIL import Image, ImageTk, ImageDraw
import json
import os
from pathlib import Path
import sys
import math

class Cartographer:
    def __init__(self, root):
        self.root = root
        self.root.title("RoH Cartographer")
        self.root.geometry("1400x900")
        
        self.current_level = None
        self.level_size = 16
        self.cell_size = 35
        self.current_layer = "wall"
        self.current_tool = "brush"  # "brush", "eraser", "player"
        self.current_texture_id = 1
        self.current_furniture = "box"
        self.textures = {}
        self.furniture_types = {}
        self.texture_images = {}
        self.furniture_images = {}
        self.texture_thumbnails = {}
        self.furniture_thumbnails = {}
        
        # Для отслеживания использованных ресурсов
        self.used_textures = {
            'wall': set(),
            'floor': set(),
            'ceiling': set()
        }
        self.used_furniture = set()  # Для отслеживания использованной мебели
        
        # Режимы размещения мебели
        self.furniture_placement_mode = "center"  # "center" или "free"
        self.dragging_furniture = None  # Для перетаскивания мебели
        self.drag_start_pos = None
        
        # Стартовая позиция игрока (согласно формату .roh)
        self.player_pos = {
            'x': 1.5,
            'y': 1.5,
            'dir_x': -1.0,
            'dir_y': 0.0
        }
        
        # Пути к ресурсам проекта
        self.project_root = Path(__file__).parent.parent
        self.textures_dir = self.project_root / "resources" / "textures"
        self.furniture_dir = self.project_root / "resources" / "furniture"
        self.sounds_dir = self.project_root / "resources" / "sounds"
        
        # Создаем директорию для сохранения если её нет
        self.levels_dir = "levels"
        os.makedirs(self.levels_dir, exist_ok=True)
        
        self.setup_ui()
        self.create_new_level(16, 16)
        self.load_resources()
        
    def setup_ui(self):
        # Главный контейнер
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # Левая панель инструментов с вкладками
        left_notebook = ttk.Notebook(main_frame, width=300)
        left_notebook.pack(side=tk.LEFT, fill=tk.Y, padx=5, pady=5)
        
        # Вкладка "Инструменты"
        tools_tab = ttk.Frame(left_notebook)
        left_notebook.add(tools_tab, text="Инструменты")
        self.setup_tools_tab(tools_tab)
        
        # Вкладка "Текстуры"
        textures_tab = ttk.Frame(left_notebook)
        left_notebook.add(textures_tab, text="Текстуры")
        self.setup_textures_tab(textures_tab)
        
        # Вкладка "Мебель"
        furniture_tab = ttk.Frame(left_notebook)
        left_notebook.add(furniture_tab, text="Мебель")
        self.setup_furniture_tab(furniture_tab)
        
        # Вкладка "Звуки"
        sounds_tab = ttk.Frame(left_notebook)
        left_notebook.add(sounds_tab, text="Звуки")
        self.setup_sounds_tab(sounds_tab)
        
        # Правая панель с канвасом
        right_panel = ttk.Frame(main_frame)
        right_panel.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)
        
        # Верхняя панель с кнопками и информацией
        top_panel = ttk.Frame(right_panel)
        top_panel.pack(fill=tk.X, padx=5, pady=5)
        
        # Кнопки управления уровнем
        btn_frame = ttk.Frame(top_panel)
        btn_frame.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 20))
        
        ttk.Button(btn_frame, text="Новый уровень", command=self.new_level, 
                  width=15).pack(pady=2)
        ttk.Button(btn_frame, text="Загрузить уровень", command=self.load_level,
                  width=15).pack(pady=2)
        ttk.Button(btn_frame, text="Сохранить уровень", command=self.save_level,
                  width=15).pack(pady=2)
        ttk.Button(btn_frame, text="Сохранить как...", command=self.save_level_as,
                  width=15).pack(pady=2)
        
        # Панель информации
        info_frame = ttk.Frame(top_panel)
        info_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        # Первая строка информации
        info_row1 = ttk.Frame(info_frame)
        info_row1.pack(fill=tk.X, pady=2)
        
        ttk.Label(info_row1, text="Уровень:").pack(side=tk.LEFT)
        self.level_name_label = ttk.Label(info_row1, text="Новый уровень", 
                                         font=("Arial", 10, "bold"))
        self.level_name_label.pack(side=tk.LEFT, padx=(0, 20))
        
        ttk.Label(info_row1, text="Размер:").pack(side=tk.LEFT)
        self.size_label = ttk.Label(info_row1, text="16x16")
        self.size_label.pack(side=tk.LEFT, padx=(0, 20))
        
        # Вторая строка информации
        info_row2 = ttk.Frame(info_frame)
        info_row2.pack(fill=tk.X, pady=2)
        
        ttk.Label(info_row2, text="Слой:").pack(side=tk.LEFT)
        self.layer_label = ttk.Label(info_row2, text="Стены")
        self.layer_label.pack(side=tk.LEFT, padx=(0, 20))
        
        ttk.Label(info_row2, text="Инструмент:").pack(side=tk.LEFT)
        self.tool_label = ttk.Label(info_row2, text="Кисть")
        self.tool_label.pack(side=tk.LEFT, padx=(0, 20))
        
        ttk.Label(info_row2, text="Координаты:").pack(side=tk.LEFT)
        self.coord_label = ttk.Label(info_row2, text="(0, 0)")
        self.coord_label.pack(side=tk.LEFT)
        
        # Третья строка - информация об игроке
        info_row3 = ttk.Frame(info_frame)
        info_row3.pack(fill=tk.X, pady=2)
        
        ttk.Label(info_row3, text="Игрок:").pack(side=tk.LEFT)
        self.player_info_label = ttk.Label(info_row3, text="X: 1.5, Y: 1.5, Направление: ←")
        self.player_info_label.pack(side=tk.LEFT)
        
        # Четвертая строка - режим размещения мебели
        info_row4 = ttk.Frame(info_frame)
        info_row4.pack(fill=tk.X, pady=2)
        
        ttk.Label(info_row4, text="Мебель:").pack(side=tk.LEFT)
        self.furniture_mode_label = ttk.Label(info_row4, text="Режим: центр клетки")
        self.furniture_mode_label.pack(side=tk.LEFT)
        
        # Канвас для карты с прокруткой
        canvas_frame = ttk.Frame(right_panel)
        canvas_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Добавляем скроллбары
        canvas_vscroll = ttk.Scrollbar(canvas_frame, orient=tk.VERTICAL)
        canvas_hscroll = ttk.Scrollbar(canvas_frame, orient=tk.HORIZONTAL)
        
        self.canvas = tk.Canvas(canvas_frame, bg='#2E2E2E',
                               yscrollcommand=canvas_vscroll.set,
                               xscrollcommand=canvas_hscroll.set)
        
        canvas_vscroll.config(command=self.canvas.yview)
        canvas_hscroll.config(command=self.canvas.xview)
        
        # Размещаем элементы
        canvas_hscroll.pack(side=tk.BOTTOM, fill=tk.X)
        canvas_vscroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        self.canvas.bind("<Button-1>", self.on_canvas_click)
        self.canvas.bind("<B1-Motion>", self.on_canvas_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_canvas_release)
        self.canvas.bind("<MouseWheel>", self.on_mouse_wheel)
        self.canvas.bind("<Motion>", self.on_canvas_motion)
        
        # Обработка прокрутки при наведении на canvas
        self.canvas.bind("<Enter>", lambda e: self.canvas.focus_set())
        
    def setup_tools_tab(self, parent):
        # Панель инструментов редактирования
        edit_tools_frame = ttk.LabelFrame(parent, text="Инструменты редактирования", padding=10)
        edit_tools_frame.pack(fill=tk.X, pady=(0, 10))
        
        self.tool_var = tk.StringVar(value="brush")
        
        # Создаем иконки для инструментов
        tool_icons = self.create_tool_icons()
        
        # Кисть
        brush_frame = ttk.Frame(edit_tools_frame)
        brush_frame.pack(fill=tk.X, pady=2)
        ttk.Radiobutton(brush_frame, text="Кисть", variable=self.tool_var, 
                       value="brush", command=self.on_tool_change).pack(side=tk.LEFT)
        if tool_icons.get("brush"):
            brush_icon = tk.Label(brush_frame, image=tool_icons["brush"])
            brush_icon.image = tool_icons["brush"]  # Сохраняем ссылку
            brush_icon.pack(side=tk.RIGHT)
        
        # Ластик
        eraser_frame = ttk.Frame(edit_tools_frame)
        eraser_frame.pack(fill=tk.X, pady=2)
        ttk.Radiobutton(eraser_frame, text="Ластик", variable=self.tool_var,
                       value="eraser", command=self.on_tool_change).pack(side=tk.LEFT)
        if tool_icons.get("eraser"):
            eraser_icon = tk.Label(eraser_frame, image=tool_icons["eraser"])
            eraser_icon.image = tool_icons["eraser"]
            eraser_icon.pack(side=tk.RIGHT)
        
        # Инструмент игрока
        player_frame = ttk.Frame(edit_tools_frame)
        player_frame.pack(fill=tk.X, pady=2)
        ttk.Radiobutton(player_frame, text="Игрок", variable=self.tool_var,
                       value="player", command=self.on_tool_change).pack(side=tk.LEFT)
        if tool_icons.get("player"):
            player_icon = tk.Label(player_frame, image=tool_icons["player"])
            player_icon.image = tool_icons["player"]
            player_icon.pack(side=tk.RIGHT)
        
        # Панель управления слоями
        layer_frame = ttk.LabelFrame(parent, text="Слои", padding=10)
        layer_frame.pack(fill=tk.X, pady=(0, 10))
        
        self.layer_var = tk.StringVar(value="wall")
        layers = [("Стены", "wall"), ("Пол", "floor"), ("Потолок", "ceiling"), ("Мебель", "furniture")]
        for text, value in layers:
            ttk.Radiobutton(layer_frame, text=text, variable=self.layer_var, 
                          value=value, command=self.on_layer_change).pack(anchor=tk.W)
        
        # Режимы размещения мебели
        furniture_frame = ttk.LabelFrame(parent, text="Режим мебели", padding=10)
        furniture_frame.pack(fill=tk.X, pady=(0, 10))
        
        self.furniture_mode_var = tk.StringVar(value="center")
        ttk.Radiobutton(furniture_frame, text="Центр клетки", variable=self.furniture_mode_var,
                       value="center", command=self.on_furniture_mode_change).pack(anchor=tk.W)
        ttk.Radiobutton(furniture_frame, text="Свободное размещение", variable=self.furniture_mode_var,
                       value="free", command=self.on_furniture_mode_change).pack(anchor=tk.W)
        
        # Кнопка изменения свойств уровня
        ttk.Button(parent, text="Свойства уровня...", 
                  command=self.edit_level_properties).pack(pady=10)
        
    def on_furniture_mode_change(self):
        self.furniture_placement_mode = self.furniture_mode_var.get()
        if self.furniture_placement_mode == "center":
            self.furniture_mode_label.config(text="Режим: центр клетки")
        else:
            self.furniture_mode_label.config(text="Режим: свободное размещение")
        
    def create_tool_icons(self):
        """Создает иконки для инструментов"""
        icons = {}
        
        # Иконка кисти
        brush_img = Image.new('RGBA', (20, 20), (255, 255, 255, 0))
        draw = ImageDraw.Draw(brush_img)
        draw.ellipse((2, 2, 18, 18), fill='#4CAF50', outline='#2E7D32', width=2)
        brush_icon = ImageTk.PhotoImage(brush_img)
        icons["brush"] = brush_icon
        
        # Иконка ластика
        eraser_img = Image.new('RGBA', (20, 20), (255, 255, 255, 0))
        draw = ImageDraw.Draw(eraser_img)
        draw.rectangle((2, 2, 18, 18), fill='#F44336', outline='#C62828', width=2)
        eraser_icon = ImageTk.PhotoImage(eraser_img)
        icons["eraser"] = eraser_icon
        
        # Иконка игрока
        player_img = Image.new('RGBA', (20, 20), (255, 255, 255, 0))
        draw = ImageDraw.Draw(player_img)
        draw.ellipse((2, 2, 18, 18), fill='#2196F3', outline='#1565C0', width=2)
        # Стрелка направления
        draw.line((10, 10, 18, 10), fill='#FFEB3B', width=2)
        player_icon = ImageTk.PhotoImage(player_img)
        icons["player"] = player_icon
        
        return icons
        
    def setup_textures_tab(self, parent):
        # Холст для предпросмотра текущей текстуры
        preview_frame = ttk.LabelFrame(parent, text="Выбранная текстура", padding=10)
        preview_frame.pack(fill=tk.X, pady=(0, 10))
        
        self.current_texture_preview = tk.Canvas(preview_frame, height=80, bg='gray')
        self.current_texture_preview.pack(fill=tk.X)
        
        # Информация о текстуре
        self.texture_info_label = ttk.Label(preview_frame, text="ID: 1 - Пусто")
        self.texture_info_label.pack()
        
        # Список текстур с прокруткой
        list_frame = ttk.LabelFrame(parent, text="Доступные текстуры", padding=10)
        list_frame.pack(fill=tk.BOTH, expand=True)
        
        texture_container = ttk.Frame(list_frame)
        texture_container.pack(fill=tk.BOTH, expand=True)
        
        texture_scrollbar = ttk.Scrollbar(texture_container)
        texture_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.texture_listbox = tk.Listbox(texture_container, height=20, 
                                         yscrollcommand=texture_scrollbar.set)
        self.texture_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        texture_scrollbar.config(command=self.texture_listbox.yview)
        self.texture_listbox.bind('<<ListboxSelect>>', self.on_texture_select)
        
    def setup_furniture_tab(self, parent):
        # Холст для предпросмотра текущей мебели
        preview_frame = ttk.LabelFrame(parent, text="Выбранная мебель", padding=10)
        preview_frame.pack(fill=tk.X, pady=(0, 10))
        
        self.current_furniture_preview = tk.Canvas(preview_frame, height=80, bg='gray')
        self.current_furniture_preview.pack(fill=tk.X)
        
        # Информация о мебели
        self.furniture_info_label = ttk.Label(preview_frame, text="box - Ящик")
        self.furniture_info_label.pack()
        
        # Список мебели с прокруткой
        list_frame = ttk.LabelFrame(parent, text="Доступная мебель", padding=10)
        list_frame.pack(fill=tk.BOTH, expand=True)
        
        furniture_container = ttk.Frame(list_frame)
        furniture_container.pack(fill=tk.BOTH, expand=True)
        
        furniture_scrollbar = ttk.Scrollbar(furniture_container)
        furniture_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.furniture_listbox = tk.Listbox(furniture_container, height=20, 
                                           yscrollcommand=furniture_scrollbar.set)
        self.furniture_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        furniture_scrollbar.config(command=self.furniture_listbox.yview)
        self.furniture_listbox.bind('<<ListboxSelect>>', self.on_furniture_select)
        
    def setup_sounds_tab(self, parent):
        # Фоновая музыка
        music_frame = ttk.LabelFrame(parent, text="Фоновая музыка", padding=10)
        music_frame.pack(fill=tk.X, pady=(0, 10))
        
        music_row = ttk.Frame(music_frame)
        music_row.pack(fill=tk.X)
        
        self.music_var = tk.StringVar(value="")
        music_entry = ttk.Entry(music_row, textvariable=self.music_var, width=30)
        music_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 5))
        
        ttk.Button(music_row, text="Обзор...", 
                  command=self.browse_music).pack(side=tk.RIGHT)
        
        # Эмбиент
        ambience_frame = ttk.LabelFrame(parent, text="Фоновый эмбиент", padding=10)
        ambience_frame.pack(fill=tk.X, pady=(0, 10))
        
        ambience_row = ttk.Frame(ambience_frame)
        ambience_row.pack(fill=tk.X)
        
        self.ambience_var = tk.StringVar(value="")
        ambience_entry = ttk.Entry(ambience_row, textvariable=self.ambience_var, width=30)
        ambience_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 5))
        
        ttk.Button(ambience_row, text="Обзор...", 
                  command=self.browse_ambience).pack(side=tk.RIGHT)
        
        # Кнопка применения звуков
        ttk.Button(parent, text="Применить звуки к уровню", 
                  command=self.apply_sounds_to_level).pack(pady=10)
        
    def browse_music(self):
        initial_dir = self.sounds_dir / "music" if self.sounds_dir.exists() else "."
        file_path = filedialog.askopenfilename(
            title="Выберите фоновую музыку",
            initialdir=str(initial_dir),
            filetypes=[("MIDI files", "*.mid"), ("WAV files", "*.wav"), 
                      ("All files", "*.*")]
        )
        
        if file_path:
            # Преобразуем путь в относительный от resources
            rel_path = self.make_relative_path(file_path)
            self.music_var.set(rel_path)
    
    def browse_ambience(self):
        initial_dir = self.sounds_dir / "effects" if self.sounds_dir.exists() else "."
        file_path = filedialog.askopenfilename(
            title="Выберите фоновый эмбиент",
            initialdir=str(initial_dir),
            filetypes=[("WAV files", "*.wav"), ("All files", "*.*")]
        )
        
        if file_path:
            # Преобразуем путь в относительный от resources
            rel_path = self.make_relative_path(file_path)
            self.ambience_var.set(rel_path)
    
    def make_relative_path(self, absolute_path):
        """Преобразует абсолютный путь в относительный от папки resources"""
        try:
            # Пытаемся сделать путь относительным от project_root
            relative_path = Path(absolute_path).relative_to(self.project_root)
            return str(relative_path)
        except ValueError:
            # Если не удается, возвращаем путь как есть
            return absolute_path
    
    def apply_sounds_to_level(self):
        if not self.current_level:
            return
            
        # Обновляем текущий уровень с выбранными звуками
        self.current_level['background_music'] = self.music_var.get()
        self.current_level['ambience_sound'] = self.ambience_var.get()
        
        messagebox.showinfo("Успех", "Звуки применены к уровню")
    
    def create_new_level(self, width=16, height=16):
        # Сохраняем отдельно ширину и высоту
        self.level_width = width
        self.level_height = height
        # Для совместимости со старым кодом
        self.level_size = max(width, height)
        
        self.current_level = {
            'name': 'Новый уровень',
            'width': width,
            'height': height,
            'wall': [[0 for _ in range(width)] for _ in range(height)],
            'floor': [[0 for _ in range(width)] for _ in range(height)],
            'ceiling': [[0 for _ in range(width)] for _ in range(height)],
            'furniture': [],
            'background_music': '',
            'ambience_sound': '',
            'textures': {
                'wall': {},
                'floor': {},
                'ceiling': {}
            }
        }
        # Сбрасываем позицию игрока
        self.player_pos = {
            'x': 1.5,
            'y': 1.5,
            'dir_x': -1.0,
            'dir_y': 0.0
        }
        # Сбрасываем использованные ресурсы
        self.used_textures = {
            'wall': set(),
            'floor': set(),
            'ceiling': set()
        }
        self.used_furniture = set()
        self.update_level_info()
        self.redraw_canvas()
        
    def update_used_resources(self):
        """Обновляет набор использованных текстур и мебели в уровне"""
        if not self.current_level:
            return
            
        # Очищаем наборы
        for layer in self.used_textures:
            self.used_textures[layer].clear()
        self.used_furniture.clear()
        
        # Собираем использованные текстуры
        for y in range(self.current_level['height']):
            for x in range(self.current_level['width']):
                tex_id = self.current_level['wall'][y][x]
                if tex_id > 0:
                    self.used_textures['wall'].add(tex_id)
                
                tex_id = self.current_level['floor'][y][x]
                if tex_id > 0:
                    self.used_textures['floor'].add(tex_id)
                
                tex_id = self.current_level['ceiling'][y][x]
                if tex_id > 0:
                    self.used_textures['ceiling'].add(tex_id)
        
        # Собираем использованную мебель
        for furniture in self.current_level['furniture']:
            self.used_furniture.add(furniture['type'])
    
    def update_level_info(self):
        """Обновляет информацию об уровне в интерфейсе"""
        if self.current_level:
            self.level_name_label.config(text=self.current_level['name'])
            self.size_label.config(text=f"{self.current_level['width']}x{self.current_level['height']}")
            
            # Обновляем звуки
            self.music_var.set(self.current_level.get('background_music', ''))
            self.ambience_var.set(self.current_level.get('ambience_sound', ''))
            
            # Обновляем информацию об игроке
            direction = self.get_direction_name(self.player_pos['dir_x'], self.player_pos['dir_y'])
            self.player_info_label.config(
                text=f"X: {self.player_pos['x']:.1f}, Y: {self.player_pos['y']:.1f}, Направление: {direction}"
            )
    
    def get_direction_name(self, dir_x, dir_y):
        """Возвращает название направления по вектору"""
        # В компьютерной графике Y растет вниз, поэтому инвертируем Y
        # для правильного отображения направлений
        screen_dir_y = -dir_y  # Инвертируем Y для правильного отображения
        
        # Нормализуем вектор
        length = math.sqrt(dir_x**2 + screen_dir_y**2)
        if length > 0:
            dir_x /= length
            screen_dir_y /= length
        
        # Определяем направление
        angle = math.degrees(math.atan2(screen_dir_y, dir_x))
        
        if -22.5 <= angle < 22.5:
            return "→"  # Вправо
        elif 22.5 <= angle < 67.5:
            return "↗"  # Вверх-вправо
        elif 67.5 <= angle < 112.5:
            return "↑"  # Вверх
        elif 112.5 <= angle < 157.5:
            return "↖"  # Вверх-влево
        elif angle >= 157.5 or angle < -157.5:
            return "←"  # Влево
        elif -157.5 <= angle < -112.5:
            return "↙"  # Вниз-влево
        elif -112.5 <= angle < -67.5:
            return "↓"  # Вниз
        elif -67.5 <= angle < -22.5:
            return "↘"  # Вниз-вправо
        
        return "?"
        
    def edit_level_properties(self):
        if not self.current_level:
            return
            
        # Создаем диалоговое окно для редактирования свойств уровня
        dialog = tk.Toplevel(self.root)
        dialog.title("Свойства уровня")
        dialog.geometry("450x400")  # Увеличили высоту
        dialog.transient(self.root)
        dialog.grab_set()
        
        # Создаем Notebook для вкладок
        notebook = ttk.Notebook(dialog)
        notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Вкладка "Основное"
        basic_tab = ttk.Frame(notebook)
        notebook.add(basic_tab, text="Основное")
        
        # Вкладка "Звуки"
        sounds_tab = ttk.Frame(notebook)
        notebook.add(sounds_tab, text="Звуки")
        
        # Основные настройки
        ttk.Label(basic_tab, text="Название уровня:").pack(anchor=tk.W, padx=20, pady=(20, 5))
        name_var = tk.StringVar(value=self.current_level['name'])
        name_entry = ttk.Entry(basic_tab, textvariable=name_var, width=40)
        name_entry.pack(padx=20, pady=(0, 15))
        
        # Размер уровня
        ttk.Label(basic_tab, text="Размер уровня:").pack(anchor=tk.W, padx=20, pady=5)
        size_frame = ttk.Frame(basic_tab)
        size_frame.pack(padx=20, pady=(0, 15))
        
        width_var = tk.StringVar(value=str(self.current_level['width']))
        height_var = tk.StringVar(value=str(self.current_level['height']))
        
        ttk.Label(size_frame, text="Ширина:").pack(side=tk.LEFT)
        width_spin = ttk.Spinbox(size_frame, from_=8, to=128, textvariable=width_var, width=8)
        width_spin.pack(side=tk.LEFT, padx=(5, 20))
        
        ttk.Label(size_frame, text="Высота:").pack(side=tk.LEFT)
        height_spin = ttk.Spinbox(size_frame, from_=8, to=128, textvariable=height_var, width=8)
        height_spin.pack(side=tk.LEFT, padx=(5, 0))
        
        # Позиция игрока
        ttk.Label(basic_tab, text="Позиция игрока:").pack(anchor=tk.W, padx=20, pady=5)
        pos_frame = ttk.Frame(basic_tab)
        pos_frame.pack(padx=20, pady=(0, 10))
        
        ttk.Label(pos_frame, text="X:").pack(side=tk.LEFT)
        player_x_var = tk.StringVar(value=str(self.player_pos['x']))
        player_x_spin = ttk.Spinbox(pos_frame, from_=0.5, to=self.current_level['width']-0.5, 
                                   increment=0.5, textvariable=player_x_var, width=8)
        player_x_spin.pack(side=tk.LEFT, padx=(5, 20))
        
        ttk.Label(pos_frame, text="Y:").pack(side=tk.LEFT)
        player_y_var = tk.StringVar(value=str(self.player_pos['y']))
        player_y_spin = ttk.Spinbox(pos_frame, from_=0.5, to=self.current_level['height']-0.5,
                                   increment=0.5, textvariable=player_y_var, width=8)
        player_y_spin.pack(side=tk.LEFT, padx=(5, 0))
        
        # Направление игрока
        ttk.Label(basic_tab, text="Направление игрока:").pack(anchor=tk.W, padx=20, pady=5)
        dir_frame = ttk.Frame(basic_tab)
        dir_frame.pack(padx=20, pady=(0, 10))
        
        ttk.Label(dir_frame, text="X:").pack(side=tk.LEFT)
        player_dir_x_var = tk.StringVar(value=str(self.player_pos['dir_x']))
        player_dir_x_spin = ttk.Spinbox(dir_frame, from_=-1.0, to=1.0, 
                                       increment=0.1, textvariable=player_dir_x_var, width=8)
        player_dir_x_spin.pack(side=tk.LEFT, padx=(5, 20))
        
        ttk.Label(dir_frame, text="Y:").pack(side=tk.LEFT)
        player_dir_y_var = tk.StringVar(value=str(self.player_pos['dir_y']))
        player_dir_y_spin = ttk.Spinbox(dir_frame, from_=-1.0, to=1.0,
                                       increment=0.1, textvariable=player_dir_y_var, width=8)
        player_dir_y_spin.pack(side=tk.LEFT, padx=(5, 0))
        
        # Вкладка звуков
        ttk.Label(sounds_tab, text="Фоновая музыка:").pack(anchor=tk.W, padx=20, pady=(20, 5))
        music_frame = ttk.Frame(sounds_tab)
        music_frame.pack(fill=tk.X, padx=20, pady=(0, 15))
        
        music_var_dialog = tk.StringVar(value=self.current_level.get('background_music', ''))
        music_entry = ttk.Entry(music_frame, textvariable=music_var_dialog, width=30)
        music_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 5))
        
        def browse_music_dialog():
            initial_dir = self.sounds_dir / "music" if self.sounds_dir.exists() else "."
            file_path = filedialog.askopenfilename(
                title="Выберите фоновую музыку",
                initialdir=str(initial_dir),
                filetypes=[("MIDI files", "*.mid"), ("WAV files", "*.wav"), 
                          ("All files", "*.*")]
            )
            
            if file_path:
                rel_path = self.make_relative_path(file_path)
                music_var_dialog.set(rel_path)
        
        ttk.Button(music_frame, text="Обзор...", command=browse_music_dialog).pack(side=tk.RIGHT)
        
        ttk.Label(sounds_tab, text="Фоновый эмбиент:").pack(anchor=tk.W, padx=20, pady=5)
        ambience_frame = ttk.Frame(sounds_tab)
        ambience_frame.pack(fill=tk.X, padx=20, pady=(0, 15))
        
        ambience_var_dialog = tk.StringVar(value=self.current_level.get('ambience_sound', ''))
        ambience_entry = ttk.Entry(ambience_frame, textvariable=ambience_var_dialog, width=30)
        ambience_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 5))
        
        def browse_ambience_dialog():
            initial_dir = self.sounds_dir / "effects" if self.sounds_dir.exists() else "."
            file_path = filedialog.askopenfilename(
                title="Выберите фоновый эмбиент",
                initialdir=str(initial_dir),
                filetypes=[("WAV files", "*.wav"), ("All files", "*.*")]
            )
            
            if file_path:
                rel_path = self.make_relative_path(file_path)
                ambience_var_dialog.set(rel_path)
        
        ttk.Button(ambience_frame, text="Обзор...", command=browse_ambience_dialog).pack(side=tk.RIGHT)
        
        # Кнопки
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(fill=tk.X, padx=20, pady=20)
        
        def save_properties():
            try:
                # Сохраняем размер
                new_width = int(width_var.get())
                new_height = int(height_var.get())
                
                if new_width != self.current_level['width'] or new_height != self.current_level['height']:
                    self.resize_level(new_width, new_height)
                
                # Сохраняем имя
                self.current_level['name'] = name_var.get()
                
                # Сохраняем позицию и направление игрока
                self.player_pos['x'] = float(player_x_var.get())
                self.player_pos['y'] = float(player_y_var.get())
                self.player_pos['dir_x'] = float(player_dir_x_var.get())
                self.player_pos['dir_y'] = float(player_dir_y_var.get())
                
                # Сохраняем звуки
                self.current_level['background_music'] = music_var_dialog.get()
                self.current_level['ambience_sound'] = ambience_var_dialog.get()
                
                # Обновляем UI звуков
                self.music_var.set(music_var_dialog.get())
                self.ambience_var.set(ambience_var_dialog.get())
                
                self.update_level_info()
                self.redraw_canvas()
                dialog.destroy()
                
            except ValueError as e:
                messagebox.showerror("Ошибка", f"Некорректное значение: {e}")
        
        ttk.Button(btn_frame, text="Сохранить", command=save_properties).pack(side=tk.RIGHT, padx=5)
        ttk.Button(btn_frame, text="Отмена", command=dialog.destroy).pack(side=tk.RIGHT)
    
    def resize_level(self, new_width, new_height):
        """Изменяет размер уровня"""
        old_width = self.current_level['width']
        old_height = self.current_level['height']
        
        self.level_width = new_width
        self.level_height = new_height
        # Для совместимости
        self.level_size = max(new_width, new_height)
        
        # Создаем новые слои
        new_walls = [[0 for _ in range(new_width)] for _ in range(new_height)]
        new_floors = [[0 for _ in range(new_width)] for _ in range(new_height)]
        new_ceilings = [[0 for _ in range(new_width)] for _ in range(new_height)]
        
        # Копируем старые данные
        for y in range(min(old_height, new_height)):
            for x in range(min(old_width, new_width)):
                new_walls[y][x] = self.current_level['wall'][y][x]
                new_floors[y][x] = self.current_level['floor'][y][x]
                new_ceilings[y][x] = self.current_level['ceiling'][y][x]
        
        # Обновляем мебель (удаляем ту, что за пределами нового размера)
        new_furniture = []
        for furniture in self.current_level['furniture']:
            x, y = furniture['x'], furniture['y']
            if 0 <= x < new_width and 0 <= y < new_height:
                new_furniture.append(furniture)
        
        # Обновляем уровень
        self.current_level['wall'] = new_walls
        self.current_level['floor'] = new_floors
        self.current_level['ceiling'] = new_ceilings
        self.current_level['furniture'] = new_furniture
        self.current_level['width'] = new_width
        self.current_level['height'] = new_height
        
        # Проверяем позицию игрока
        if self.player_pos['x'] >= new_width:
            self.player_pos['x'] = new_width - 0.5
        if self.player_pos['y'] >= new_height:
            self.player_pos['y'] = new_height - 0.5
        
        # Обновляем использованные ресурсы
        self.update_used_resources()
    
    def load_resources(self):
        """Загружает текстуры и мебель из папок проекта"""
        self.load_textures()
        self.load_furniture()
        self.update_texture_list()
        self.update_furniture_list()
        
        # Устанавливаем первую текстуру и мебель по умолчанию
        if self.texture_listbox.size() > 0:
            self.texture_listbox.selection_set(0)
            self.on_texture_select(None)
        
        if self.furniture_listbox.size() > 0:
            self.furniture_listbox.selection_set(0)
            self.on_furniture_select(None)
    
    def load_textures(self):
        """Загружает текстуры из папок проекта"""
        self.textures = {}
        self.texture_images = {}
        self.texture_thumbnails = {}
        
        # Базовые текстуры (запасные, если папки не существуют)
        fallback_textures = {
            0: ("Пусто", "#2E2E2E", None, None),
            1: ("Кирпичи", "#FF6B6B", None, None),
            2: ("Доски", "#8B4513", None, None),
            3: ("Камень", "#808080", None, None),
            4: ("Металл", "#C0C0C0", None, None),
            5: ("Земля", "#8B7355", None, None),
            6: ("Трава", "#7CFC00", None, None),
            7: ("Вода", "#1E90FF", None, None),
        }
        
        # Загружаем реальные текстуры из папки textures/surfaces
        textures_path = self.textures_dir / "surfaces"
        if textures_path.exists():
            texture_id = 1
            for file in sorted(textures_path.glob("*.png")):
                try:
                    # Открываем оригинальное изображение
                    img = Image.open(file)
                    
                    # Создаем миниатюру для списка (64x64)
                    thumbnail_64 = img.resize((64, 64), Image.Resampling.LANCZOS)
                    photo_64 = ImageTk.PhotoImage(thumbnail_64)
                    
                    name = file.stem.replace("_", " ").title()
                    self.textures[texture_id] = (name, "#FFFFFF", photo_64, file)
                    self.texture_images[texture_id] = photo_64
                    
                    # Создаем миниатюру для отображения в клетках
                    self.texture_thumbnails[texture_id] = img.copy()
                    
                    texture_id += 1
                except Exception as e:
                    print(f"Ошибка загрузки текстуры {file}: {e}")
        else:
            # Используем запасные текстуры
            for tex_id, (name, color, _, _) in fallback_textures.items():
                self.textures[tex_id] = (name, color, None, None)
    
    def load_furniture(self):
        """Загружает информацию о мебели из .fur файлов"""
        self.furniture_types = {}
        self.furniture_images = {}
        self.furniture_thumbnails = {}
        
        # Загружаем .fur файлы
        if self.furniture_dir.exists():
            for file in sorted(self.furniture_dir.glob("*.fur")):
                try:
                    fur_type = file.stem
                    texture_path = None
                    
                    # Парсим .fur файл
                    with open(file, 'r', encoding='utf-8') as f:
                        for line in f:
                            line = line.strip()
                            if line.startswith('texture '):
                                texture_path = line.split(' ', 1)[1]
                                break
                    
                    # Загружаем изображение мебели
                    photo_64 = None
                    original_img = None
                    if texture_path:
                        # Ищем текстуру мебели
                        texture_file = self.project_root / "resources" / texture_path
                        if texture_file.exists():
                            img = Image.open(texture_file)
                            
                            # Создаем миниатюру для списка
                            thumbnail_64 = img.resize((64, 64), Image.Resampling.LANCZOS)
                            photo_64 = ImageTk.PhotoImage(thumbnail_64)
                            original_img = img.copy()
                    
                    name = fur_type.replace("_", " ").title()
                    self.furniture_types[fur_type] = (name, "#D2691E", photo_64, file)
                    if photo_64:
                        self.furniture_images[fur_type] = photo_64
                    if original_img:
                        self.furniture_thumbnails[fur_type] = original_img
                        
                except Exception as e:
                    print(f"Ошибка загрузки мебели {file}: {e}")
    
    def update_texture_list(self):
        self.texture_listbox.delete(0, tk.END)
        for tex_id, (name, color, photo, path) in sorted(self.textures.items()):
            self.texture_listbox.insert(tk.END, f"{tex_id}: {name}")
    
    def update_furniture_list(self):
        self.furniture_listbox.delete(0, tk.END)
        for fur_id, (name, color, photo, path) in sorted(self.furniture_types.items()):
            self.furniture_listbox.insert(tk.END, f"{name} ({fur_id})")
    
    def on_tool_change(self):
        self.current_tool = self.tool_var.get()
        if self.current_tool == "brush":
            self.tool_label.config(text="Кисть")
        elif self.current_tool == "eraser":
            self.tool_label.config(text="Ластик")
        elif self.current_tool == "player":
            self.tool_label.config(text="Игрок")
        self.redraw_canvas()
    
    def on_layer_change(self):
        layer = self.layer_var.get()
        layer_names = {
            "wall": "Стены",
            "floor": "Пол", 
            "ceiling": "Потолок",
            "furniture": "Мебель"
        }
        self.layer_label.config(text=layer_names.get(layer, "Неизвестно"))
        self.redraw_canvas()
    
    def on_texture_select(self, event):
        selection = self.texture_listbox.curselection()
        if selection:
            text = self.texture_listbox.get(selection[0])
            tex_id = int(text.split(':')[0])
            self.current_texture_id = tex_id
            
            # Обновляем предпросмотр текстуры
            self.current_texture_preview.delete("all")
            self.texture_info_label.config(text=f"ID: {tex_id} - {text.split(': ')[1]}")
            
            if tex_id in self.textures:
                name, color, photo, path = self.textures[tex_id]
                if photo:
                    # Отображаем изображение текстуры
                    self.current_texture_preview.create_image(
                        40, 40, image=photo, anchor=tk.CENTER
                    )
                else:
                    # Отображаем цветной квадрат
                    self.current_texture_preview.create_rectangle(
                        10, 10, 70, 70, fill=color, outline="#000000", width=2
                    )
                    self.current_texture_preview.create_text(
                        40, 40, text=str(tex_id), fill="white"
                    )
    
    def on_furniture_select(self, event):
        selection = self.furniture_listbox.curselection()
        if selection:
            text = self.furniture_listbox.get(selection[0])
            fur_id = text.split('(')[-1].rstrip(')')
            self.current_furniture = fur_id
            
            # Обновляем предпросмотр мебели
            self.current_furniture_preview.delete("all")
            furniture_name = text.split(' (')[0]
            self.furniture_info_label.config(text=f"{fur_id} - {furniture_name}")
            
            if fur_id in self.furniture_types:
                name, color, photo, path = self.furniture_types[fur_id]
                if photo:
                    # Отображаем изображение мебели
                    self.current_furniture_preview.create_image(
                        40, 40, image=photo, anchor=tk.CENTER
                    )
                else:
                    # Отображаем цветной квадрат
                    self.current_furniture_preview.create_rectangle(
                        10, 10, 70, 70, fill=color, outline="#000000", width=2
                    )
                    self.current_furniture_preview.create_text(
                        40, 40, text=fur_id[:3].upper(), fill="white"
                    )
    
    def on_canvas_click(self, event):
        if not self.current_level:
            return
            
        # Учитываем смещение от прокрутки
        canvas_x = self.canvas.canvasx(event.x)
        canvas_y = self.canvas.canvasy(event.y)
        
        x = canvas_x / self.cell_size
        y = canvas_y / self.cell_size
        
        # Проверяем клик по существующей мебели (для перетаскивания)
        if self.current_layer == "furniture" and self.current_tool == "brush":
            clicked_furniture = self.get_furniture_at_position(canvas_x, canvas_y)
            if clicked_furniture is not None:
                self.dragging_furniture = clicked_furniture
                self.drag_start_pos = (x, y, self.dragging_furniture['x'], self.dragging_furniture['y'])
                return
        
        cell_x = int(x)
        cell_y = int(y)
        
        if 0 <= cell_x < self.current_level['width'] and 0 <= cell_y < self.current_level['height']:
            if self.current_tool == "player":
                # Устанавливаем позицию игрока
                self.player_pos['x'] = x
                self.player_pos['y'] = y
                
                # Вычисляем направление от центра карты
                center_x = self.level_size / 2
                center_y = self.level_size / 2
                
                # Вектор от позиции игрока к центру карты
                dir_x = center_x - self.player_pos['x']
                dir_y = center_y - self.player_pos['y']
                
                # Нормализуем вектор
                length = math.sqrt(dir_x**2 + dir_y**2)
                if length > 0:
                    self.player_pos['dir_x'] = dir_x / length
                    self.player_pos['dir_y'] = dir_y / length
                
                self.update_level_info()
                self.redraw_canvas()
                return
            
            layer = self.layer_var.get()
            
            if layer == "furniture":
                if self.current_tool == "brush":
                    # Добавляем мебель
                    if self.furniture_placement_mode == "center":
                        # В центр клетки
                        furniture_x = cell_x + 0.5
                        furniture_y = cell_y + 0.5
                    else:
                        # Точная позиция клика
                        furniture_x = x
                        furniture_y = y
                    
                    self.current_level['furniture'].append({
                        'type': self.current_furniture,
                        'x': furniture_x,
                        'y': furniture_y,
                        'rotation': 0,
                        'scale': 1.0
                    })
                    self.used_furniture.add(self.current_furniture)
                else:  # eraser
                    # Удаляем мебель (ластик)
                    furniture_to_remove = self.get_furniture_at_position(canvas_x, canvas_y)
                    if furniture_to_remove is not None:
                        self.current_level['furniture'].remove(furniture_to_remove)
                        
                        # Обновляем список использованной мебели
                        self.update_used_resources()
            else:
                if self.current_tool == "brush":
                    # Устанавливаем текстуру
                    self.current_level[layer][cell_y][cell_x] = self.current_texture_id
                    # Добавляем текстуру в использованные
                    if self.current_texture_id > 0:
                        self.used_textures[layer].add(self.current_texture_id)
                else:  # eraser
                    # Стираем текстуру
                    self.current_level[layer][cell_y][cell_x] = 0
            
            self.redraw_canvas()
    
    def get_furniture_at_position(self, canvas_x, canvas_y, threshold=None):
        """Находит мебель в заданной позиции на canvas"""
        if threshold is None:
            threshold = self.cell_size * 0.3  # Порог для клика
        
        for furniture in self.current_level['furniture']:
            fur_x = furniture['x'] * self.cell_size
            fur_y = furniture['y'] * self.cell_size
            
            # Проверяем расстояние от клика до центра мебели
            distance = math.sqrt((canvas_x - fur_x)**2 + (canvas_y - fur_y)**2)
            if distance <= threshold:
                return furniture
        
        return None
    
    def on_canvas_drag(self, event):
        if not self.current_level or not self.dragging_furniture:
            self.on_canvas_click(event)
            return
            
        # Перетаскивание мебели
        canvas_x = self.canvas.canvasx(event.x)
        canvas_y = self.canvas.canvasy(event.y)
        
        x = canvas_x / self.cell_size
        y = canvas_y / self.cell_size
        
        # Обновляем позицию мебели
        if self.drag_start_pos:
            start_x, start_y, start_fur_x, start_fur_y = self.drag_start_pos
            delta_x = x - start_x
            delta_y = y - start_y
            
            self.dragging_furniture['x'] = start_fur_x + delta_x
            self.dragging_furniture['y'] = start_fur_y + delta_y
            
            # Ограничиваем позицию в пределах уровня
            self.dragging_furniture['x'] = max(0, min(self.dragging_furniture['x'], self.current_level['width']))
            self.dragging_furniture['y'] = max(0, min(self.dragging_furniture['y'], self.current_level['height']))
        
        self.redraw_canvas()
    
    def on_canvas_release(self, event):
        self.dragging_furniture = None
        self.drag_start_pos = None
    
    def on_canvas_motion(self, event):
        """Отображает координаты под курсором"""
        # Учитываем смещение от прокрутки
        canvas_x = self.canvas.canvasx(event.x)
        canvas_y = self.canvas.canvasy(event.y)
        
        x = canvas_x / self.cell_size
        y = canvas_y / self.cell_size
        
        cell_x = int(x)
        cell_y = int(y)
        
        if 0 <= cell_x < self.current_level['width'] and 0 <= cell_y < self.current_level['height']:
            # Отображаем точные координаты
            self.coord_label.config(text=f"({x:.2f}, {y:.2f})")
        else:
            self.coord_label.config(text="(-, -)")
    
    def on_mouse_wheel(self, event):
        # Изменение размера клетки колесиком мыши
        if event.delta > 0:
            self.cell_size = min(80, self.cell_size + 5)
        else:
            self.cell_size = max(15, self.cell_size - 5)
        self.redraw_canvas()
    
    def redraw_cell(self, x, y):
        """Перерисовывает только одну клетку (для оптимизации)"""
        if not self.current_level:
            return
            
        x1 = x * self.cell_size
        y1 = y * self.cell_size
        x2 = x1 + self.cell_size
        y2 = y1 + self.cell_size
        
        # Удаляем старую отрисовку клетки
        self.canvas.delete(f"cell_{x}_{y}")
        
        # Определяем цвет/текстуру клетки в зависимости от активного слоя
        layer = self.layer_var.get()
        tex_id = 0
        color = "#2E2E2E"
        
        if layer == "wall":
            tex_id = self.current_level['wall'][y][x]
        elif layer == "floor":
            tex_id = self.current_level['floor'][y][x]
        elif layer == "ceiling":
            tex_id = self.current_level['ceiling'][y][x]
        
        if tex_id in self.textures:
            name, color, photo, path = self.textures[tex_id]
        
        # Рисуем клетку
        cell_tag = f"cell_{x}_{y}"
        
        # Получаем ID текстуры стены для фона
        wall_id = self.current_level['wall'][y][x]
        wall_color = "#2E2E2E"  # Цвет по умолчанию для стен
        if wall_id in self.textures:
            wall_name, wall_color, wall_photo, wall_path = self.textures[wall_id]
        
        # Если редактируем пол или потолок, отображаем стены как фон
        if layer in ["floor", "ceiling"] and wall_id > 0:
            # Рисуем стену как фон (полностью непрозрачную)
            self.canvas.create_rectangle(x1, y1, x2, y2, fill=wall_color, 
                                        outline="#555555", width=1, 
                                        tags=cell_tag)
            
            # Если есть текстура стены, отображаем её
            if wall_id > 0 and wall_id in self.texture_thumbnails and wall_id in self.textures:
                try:
                    # Создаем миниатюру текстуры стены
                    img = self.texture_thumbnails[wall_id].copy()
                    thumb_size = int(self.cell_size * 0.8)
                    img = img.resize((thumb_size, thumb_size), Image.Resampling.LANCZOS)
                    photo_resized = ImageTk.PhotoImage(img)
                    
                    if not hasattr(self, 'canvas_images'):
                        self.canvas_images = []
                    self.canvas_images.append(photo_resized)
                    
                    self.canvas.create_image(x1 + self.cell_size//2, y1 + self.cell_size//2, 
                                           image=photo_resized, tags=cell_tag)
                except Exception as e:
                    print(f"Ошибка отображения текстуры стены {wall_id}: {e}")
            
            # Наложение текстуры пола/потолка поверх стены (полупрозрачное)
            if tex_id > 0:
                if tex_id in self.texture_thumbnails:
                    try:
                        img = self.texture_thumbnails[tex_id].copy()
                        thumb_size = int(self.cell_size * 0.7)
                        img = img.resize((thumb_size, thumb_size), Image.Resampling.LANCZOS)
                        
                        # Создаем полупрозрачное изображение
                        img = img.convert("RGBA")
                        transparent_img = Image.new("RGBA", img.size, (255, 255, 255, 0))
                        
                        # Создаем маску для прозрачности
                        mask = Image.new("L", img.size, 128)  # 128 = 50% прозрачность
                        
                        # Комбинируем изображения
                        img_with_alpha = Image.composite(img, transparent_img, mask)
                        
                        photo_resized = ImageTk.PhotoImage(img_with_alpha)
                        
                        if not hasattr(self, 'canvas_images'):
                            self.canvas_images = []
                        self.canvas_images.append(photo_resized)
                        
                        self.canvas.create_image(x1 + self.cell_size//2, y1 + self.cell_size//2, 
                                               image=photo_resized, tags=cell_tag)
                    except Exception as e:
                        print(f"Ошибка отображения текстуры {tex_id}: {e}")
                        # Fallback: рисуем текст с прозрачным фоном
                        self.canvas.create_rectangle(x1 + self.cell_size//4, y1 + self.cell_size//4,
                                                   x2 - self.cell_size//4, y2 - self.cell_size//4,
                                                   fill=color, outline="", tags=cell_tag)
                        self.canvas.create_text(x1 + self.cell_size//2, y1 + self.cell_size//2,
                                              text=str(tex_id), fill="white", font=("Arial", 10), 
                                              tags=cell_tag)
                else:
                    # Просто отображаем цветной квадрат с прозрачностью
                    self.canvas.create_rectangle(x1 + self.cell_size//4, y1 + self.cell_size//4,
                                               x2 - self.cell_size//4, y2 - self.cell_size//4,
                                               fill=color, outline="", tags=cell_tag)
                    self.canvas.create_text(x1 + self.cell_size//2, y1 + self.cell_size//2,
                                          text=str(tex_id), fill="white", font=("Arial", 10), 
                                          tags=cell_tag)
        # Если редактируем мебель, отображаем все слои
        elif layer == "furniture":
            # Рисуем фон - все три слоя в порядке: пол, стены, потолок
            floor_id = self.current_level['floor'][y][x]
            ceiling_id = self.current_level['ceiling'][y][x]
            
            # Сначала пол (если есть)
            floor_color = "#2E2E2E"
            if floor_id in self.textures:
                floor_name, floor_color, floor_photo, floor_path = self.textures[floor_id]
            
            # Заполняем клетку цветом пола
            self.canvas.create_rectangle(x1, y1, x2, y2, fill=floor_color, 
                                        outline="#555555", width=1, tags=cell_tag)
            
            # Если есть текстура пола, отображаем её
            if floor_id > 0 and floor_id in self.texture_thumbnails:
                try:
                    img = self.texture_thumbnails[floor_id].copy()
                    thumb_size = int(self.cell_size * 0.9)
                    img = img.resize((thumb_size, thumb_size), Image.Resampling.LANCZOS)
                    photo_resized = ImageTk.PhotoImage(img)
                    
                    if not hasattr(self, 'canvas_images'):
                        self.canvas_images = []
                    self.canvas_images.append(photo_resized)
                    
                    self.canvas.create_image(x1 + self.cell_size//2, y1 + self.cell_size//2, 
                                           image=photo_resized, tags=cell_tag)
                except Exception as e:
                    print(f"Ошибка отображения текстуры пола {floor_id}: {e}")
            
            # Затем стены (если есть)
            if wall_id > 0:
                # Отображаем стену как темный прямоугольник
                wall_darkness = "#8B0000"  # Темно-красный для визуализации стен
                if wall_id in self.textures:
                    wall_name, wall_color, wall_photo, wall_path = self.textures[wall_id]
                    wall_darkness = wall_color
                
                # Рисуем стену как оверлей
                self.canvas.create_rectangle(x1 + self.cell_size//4, y1 + self.cell_size//4,
                                           x2 - self.cell_size//4, y2 - self.cell_size//4,
                                           fill=wall_darkness, outline="", tags=cell_tag)
                
                # Если есть текстура стены, отображаем её
                if wall_id in self.texture_thumbnails:
                    try:
                        img = self.texture_thumbnails[wall_id].copy()
                        thumb_size = int(self.cell_size * 0.6)
                        img = img.resize((thumb_size, thumb_size), Image.Resampling.LANCZOS)
                        
                        # Делаем текстуру стены полупрозрачной
                        img = img.convert("RGBA")
                        transparent_img = Image.new("RGBA", img.size, (255, 255, 255, 0))
                        
                        # Создаем маску для прозрачности
                        mask = Image.new("L", img.size, 160)  # 160 = ~63% прозрачность
                        
                        # Комбинируем изображения
                        img_with_alpha = Image.composite(img, transparent_img, mask)
                        
                        photo_resized = ImageTk.PhotoImage(img_with_alpha)
                        
                        if not hasattr(self, 'canvas_images'):
                            self.canvas_images = []
                        self.canvas_images.append(photo_resized)
                        
                        self.canvas.create_image(x1 + self.cell_size//2, y1 + self.cell_size//2, 
                                               image=photo_resized, tags=cell_tag)
                    except Exception as e:
                        print(f"Ошибка отображения текстуры стены {wall_id}: {e}")
            
            # Затем потолок (если отличается от пола и есть)
            if ceiling_id > 0 and ceiling_id != floor_id:
                ceiling_color = "#2E2E2E"
                if ceiling_id in self.textures:
                    ceiling_name, ceiling_color, ceiling_photo, ceiling_path = self.textures[ceiling_id]
                
                # Рисуем потолок как маленький квадрат в углу
                self.canvas.create_rectangle(x2 - self.cell_size//3, y1,
                                           x2, y1 + self.cell_size//3,
                                           fill=ceiling_color, outline="#000000", 
                                           width=1, tags=cell_tag)
                
                # Если есть текстура потолка, отображаем её
                if ceiling_id in self.texture_thumbnails:
                    try:
                        img = self.texture_thumbnails[ceiling_id].copy()
                        thumb_size = int(self.cell_size * 0.25)
                        img = img.resize((thumb_size, thumb_size), Image.Resampling.LANCZOS)
                        photo_resized = ImageTk.PhotoImage(img)
                        
                        if not hasattr(self, 'canvas_images'):
                            self.canvas_images = []
                        self.canvas_images.append(photo_resized)
                        
                        self.canvas.create_image(x2 - thumb_size//2, y1 + thumb_size//2, 
                                               image=photo_resized, tags=cell_tag)
                    except Exception as e:
                        print(f"Ошибка отображения текстуры потолка {ceiling_id}: {e}")
        else:
            # Обычное отображение для слоя стен
            self.canvas.create_rectangle(x1, y1, x2, y2, fill=color, 
                                        outline="#555555", width=1, tags=cell_tag)
            
            # Если есть изображение текстуры и не ID 0 (пусто)
            if tex_id > 0 and tex_id in self.texture_thumbnails:
                try:
                    # Создаем миниатюру нужного размера
                    img = self.texture_thumbnails[tex_id].copy()
                    thumb_size = int(self.cell_size * 0.9)
                    img = img.resize((thumb_size, thumb_size), Image.Resampling.LANCZOS)
                    photo_resized = ImageTk.PhotoImage(img)
                    
                    if not hasattr(self, 'canvas_images'):
                        self.canvas_images = []
                    self.canvas_images.append(photo_resized)
                    
                    self.canvas.create_image(x1 + self.cell_size//2, y1 + self.cell_size//2, 
                                           image=photo_resized, tags=cell_tag)
                except Exception as e:
                    print(f"Ошибка отображения текстуры {tex_id}: {e}")
                    self.canvas.create_text(x1 + self.cell_size//2, y1 + self.cell_size//2,
                                          text=str(tex_id), fill="white", font=("Arial", 10), 
                                          tags=cell_tag)
            elif tex_id > 0:
                self.canvas.create_text(x1 + self.cell_size//2, y1 + self.cell_size//2,
                                      text=str(tex_id), fill="white", font=("Arial", 10), 
                                      tags=cell_tag)
    
    def redraw_furniture(self):
        """Перерисовывает всю мебель на холсте"""
        # Удаляем старую мебель
        self.canvas.delete("furniture")
        
        for furniture in self.current_level['furniture']:
            self.draw_furniture_item(furniture)
    
    def draw_furniture_item(self, furniture):
        """Рисует один объект мебели"""
        x = furniture['x'] * self.cell_size
        y = furniture['y'] * self.cell_size
        fur_type = furniture['type']
        
        if fur_type in self.furniture_types:
            name, color, photo, path = self.furniture_types[fur_type]
            size = self.cell_size * 0.7
            
            if fur_type in self.furniture_thumbnails:
                try:
                    img = self.furniture_thumbnails[fur_type].copy()
                    thumb_size = int(self.cell_size * 0.6)
                    img = img.resize((thumb_size, thumb_size), Image.Resampling.LANCZOS)
                    photo_resized = ImageTk.PhotoImage(img)
                    
                    if not hasattr(self, 'canvas_images'):
                        self.canvas_images = []
                    self.canvas_images.append(photo_resized)
                    
                    # Если мебель перетаскивается, делаем её полупрозрачной
                    tags = "furniture"
                    if furniture == self.dragging_furniture:
                        self.canvas.create_oval(x - thumb_size//2, y - thumb_size//2,
                                               x + thumb_size//2, y + thumb_size//2,
                                               fill='#888888', outline='#555555', width=2,
                                               stipple="gray50", tags=tags)
                    
                    self.canvas.create_image(x, y, image=photo_resized, tags=tags)
                except Exception as e:
                    print(f"Ошибка отображения мебели {fur_type}: {e}")
                    self.draw_furniture_fallback(x, y, size, color, fur_type)
            else:
                self.draw_furniture_fallback(x, y, size, color, fur_type)
    
    def draw_furniture_fallback(self, x, y, size, color, fur_type):
        """Рисует мебель цветным квадратом"""
        # Если мебель перетаскивается, делаем её полупрозрачной
        tags = "furniture"
        if any(fur['type'] == fur_type and fur.get('dragging', False) for fur in self.current_level['furniture']):
            self.canvas.create_rectangle(
                x - size/2, y - size/2,
                x + size/2, y + size/2,
                fill=color, outline="#000000", width=2,
                stipple="gray50", tags=tags
            )
        else:
            self.canvas.create_rectangle(
                x - size/2, y - size/2,
                x + size/2, y + size/2,
                fill=color, outline="#000000", width=2,
                tags=tags
            )
        self.canvas.create_text(x, y, text=fur_type[:3].upper(), 
                              fill="white", font=("Arial", 9),
                              tags=tags)
    
    def draw_player(self):
        """Рисует игрока на холсте"""
        # Удаляем старого игрока
        self.canvas.delete("player")
        
        # Координаты центра игрока
        center_x = self.player_pos['x'] * self.cell_size
        center_y = self.player_pos['y'] * self.cell_size
        
        # Размер значка игрока
        player_size = self.cell_size * 0.6
        
        # Рисуем круг игрока
        self.canvas.create_oval(
            center_x - player_size/2, center_y - player_size/2,
            center_x + player_size/2, center_y + player_size/2,
            fill='#2196F3', outline='#1565C0', width=2,
            tags="player"
        )
        
        # Рисуем стрелку направления
        dir_x, dir_y = self.player_pos['dir_x'], self.player_pos['dir_y']
        arrow_length = player_size * 0.7
        
        end_x = center_x + dir_x * arrow_length
        end_y = center_y + dir_y * arrow_length
        
        self.canvas.create_line(
            center_x, center_y, end_x, end_y,
            arrow=tk.LAST, arrowshape=(6, 8, 4),
            fill='#FFEB3B', width=2,
            tags="player"
        )
    
    def redraw_canvas(self):
        if not self.current_level:
            return
            
        self.canvas.delete("all")
        
        # Очищаем старые ссылки на изображения
        if hasattr(self, 'canvas_images'):
            self.canvas_images = []
        
        # Вычисляем общий размер карты
        total_width = self.current_level['width'] * self.cell_size
        total_height = self.current_level['height'] * self.cell_size
        
        # Устанавливаем область прокрутки
        self.canvas.config(scrollregion=(0, 0, total_width, total_height))
        
        # Рисуем сетку и клетки
        for y in range(self.current_level['height']):
            for x in range(self.current_level['width']):
                self.redraw_cell(x, y)
        
        # Рисуем мебель
        self.redraw_furniture()
        
        # Рисуем игрока
        self.draw_player()
    
    def new_level(self):
        size_str = simpledialog.askstring("Новый уровень", "Введите размер (например 16x16):", 
                                         initialvalue="16x16")
        if size_str:
            try:
                width, height = map(int, size_str.lower().split('x'))
                self.create_new_level(width, height)
            except:
                messagebox.showerror("Ошибка", "Неверный формат размера!")
    
    def load_level(self):
        file_path = filedialog.askopenfilename(
            title="Загрузить уровень",
            initialdir=self.levels_dir,
            filetypes=[("RoH Level Files", "*.roh"), ("All files", "*.*")]
        )
        
        if file_path:
            try:
                self.parse_roh_file(file_path)
                self.redraw_canvas()
                messagebox.showinfo("Успех", f"Уровень загружен из {os.path.basename(file_path)}")
            except Exception as e:
                messagebox.showerror("Ошибка", f"Не удалось загрузить уровень: {str(e)}")
    
    def save_level(self):
        if not self.current_level:
            return
        
        if hasattr(self, 'current_file_path') and self.current_file_path:
            try:
                content = self.generate_roh_content()
                with open(self.current_file_path, 'w', encoding='utf-8') as f:
                    f.write(content)
                messagebox.showinfo("Успех", f"Уровень сохранен в {os.path.basename(self.current_file_path)}")
            except Exception as e:
                messagebox.showerror("Ошибка", f"Не удалось сохранить уровень: {str(e)}")
        else:
            self.save_level_as()
    
    def save_level_as(self):
        if not self.current_level:
            return
        
        file_path = filedialog.asksaveasfilename(
            title="Сохранить уровень как",
            initialdir=self.levels_dir,
            defaultextension=".roh",
            filetypes=[("RoH Level Files", "*.roh"), ("All files", "*.*")]
        )
        
        if file_path:
            try:
                content = self.generate_roh_content()
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(content)
                self.current_file_path = file_path
                self.current_level['name'] = os.path.splitext(os.path.basename(file_path))[0]
                self.update_level_info()
                messagebox.showinfo("Успех", f"Уровень сохранен в {os.path.basename(file_path)}")
            except Exception as e:
                messagebox.showerror("Ошибка", f"Не удалось сохранить уровень: {str(e)}")
    
    def parse_roh_file(self, file_path):
        self.current_file_path = file_path
        
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        lines = content.split('\n')
        section = None
        
        # Сначала определяем размер из слоев
        width = 16
        height = 16
        
        # Ищем секции слоев для определения размера
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            if line == '[WALL_LAYER]' or line == '[FLOOR_LAYER]' or line == '[CEILING_LAYER]':
                # Считываем данные слоя
                layer_data = []
                j = i + 1
                while j < len(lines) and lines[j].strip() and not lines[j].strip().startswith('['):
                    row = list(map(int, lines[j].split()))
                    if row:
                        layer_data.append(row)
                    j += 1
                
                if layer_data:
                    height = len(layer_data)
                    if height > 0:
                        width = len(layer_data[0])
                    break
            i += 1
        
        self.create_new_level(width, height)
        
        # Сбрасываем название уровня на имя файла
        self.current_level['name'] = os.path.splitext(os.path.basename(file_path))[0]
        
        # Парсим все секции
        section = None
        current_section_data = []
        
        for line in lines:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            
            if line.startswith('[') and line.endswith(']'):
                # Сохраняем данные предыдущей секции
                if section and current_section_data:
                    self.process_section_data(section, current_section_data)
                
                section = line[1:-1].lower()
                current_section_data = []
            else:
                current_section_data.append(line)
        
        # Обрабатываем последнюю секцию
        if section and current_section_data:
            self.process_section_data(section, current_section_data)
        
        self.update_level_info()
        self.update_used_resources()
    
    def process_section_data(self, section, data_lines):
        """Обрабатывает данные секции уровня"""
        width = self.current_level['width']
        height = self.current_level['height']
        
        if section == 'level':
            for line in data_lines:
                if 'name' in line and '"' in line:
                    self.current_level['name'] = line.split('"')[1]
                elif 'background_music' in line:
                    parts = line.split()
                    if len(parts) > 1:
                        self.current_level['background_music'] = ' '.join(parts[1:])
                elif 'ambience_sound' in line:
                    parts = line.split()
                    if len(parts) > 1:
                        self.current_level['ambience_sound'] = ' '.join(parts[1:])
        
        elif section == 'wall_layer':
            for i, line in enumerate(data_lines[:height]):
                data = list(map(int, line.split()))
                if len(data) >= width:
                    self.current_level['wall'][i] = data[:width]
                    # Добавляем текстуры в использованные
                    for tex_id in data[:width]:
                        if tex_id > 0:
                            self.used_textures['wall'].add(tex_id)
        
        elif section == 'floor_layer':
            for i, line in enumerate(data_lines[:height]):
                data = list(map(int, line.split()))
                if len(data) >= width:
                    self.current_level['floor'][i] = data[:width]
                    # Добавляем текстуры в использованные
                    for tex_id in data[:width]:
                        if tex_id > 0:
                            self.used_textures['floor'].add(tex_id)
        
        elif section == 'ceiling_layer':
            for i, line in enumerate(data_lines[:height]):
                data = list(map(int, line.split()))
                if len(data) >= width:
                    self.current_level['ceiling'][i] = data[:width]
                    # Добавляем текстуры в использованные
                    for tex_id in data[:width]:
                        if tex_id > 0:
                            self.used_textures['ceiling'].add(tex_id)
        
        elif section == 'furniture_objects':
            for line in data_lines:
                if not line or line.startswith('#'):
                    continue
                    
                parts = line.split()
                if len(parts) >= 3:
                    fur_type = parts[0]
                    try:
                        x = float(parts[1])
                        y = float(parts[2])
                        rotation = float(parts[3]) if len(parts) > 3 else 0
                        scale = float(parts[4]) if len(parts) > 4 else 1.0
                        
                        if 0 <= x < width and 0 <= y < height:
                            furniture = {
                                'type': fur_type,
                                'x': x,
                                'y': y,
                                'rotation': rotation,
                                'scale': scale
                            }
                            self.current_level['furniture'].append(furniture)
                            self.used_furniture.add(fur_type)
                    except ValueError:
                        continue
        
        elif section == 'player':
            for line in data_lines:
                if line.startswith('player '):
                    parts = line.split()
                    if len(parts) >= 5:
                        try:
                            self.player_pos['x'] = float(parts[1])
                            self.player_pos['y'] = float(parts[2])
                            self.player_pos['dir_x'] = float(parts[3])
                            self.player_pos['dir_y'] = float(parts[4])
                        except ValueError:
                            pass
    
    def generate_roh_content(self):
        """Генерирует содержимое файла .roh с реальными текстурами и мебелью из уровня"""
        width = self.current_level['width']
        height = self.current_level['height']
        
        content = [
            f"# RoH Level File создан в Cartographer",
            f"# Размер: {width}x{height}",
            "",
            "[LEVEL]",
            f'name "{self.current_level["name"]}"',
        ]
        
        # Добавляем звуки если они есть
        if self.current_level.get('background_music'):
            content.append(f"background_music {self.current_level['background_music']}")
        if self.current_level.get('ambience_sound'):
            content.append(f"ambience_sound {self.current_level['ambience_sound']}")
        
        content.extend([
            "",
            "[PLAYER]",
            f"player {self.player_pos['x']} {self.player_pos['y']} "
            f"{self.player_pos['dir_x']} {self.player_pos['dir_y']}",
            ""
        ])
        
        # Динамически генерируем секции текстур на основе использованных
        # Стены
        if self.used_textures['wall']:
            content.append("[WALL_TEXTURES]")
            content.append("# Формат: ID путь_к_текстуре")
            for tex_id in sorted(self.used_textures['wall']):
                if tex_id in self.textures:
                    name, color, photo, path = self.textures[tex_id]
                    if path:
                        # Преобразуем путь в относительный от project_root
                        try:
                            rel_path = path.relative_to(self.project_root)
                            content.append(f"{tex_id} {rel_path}")
                        except:
                            content.append(f"{tex_id} {path}")
                    else:
                        # Для запасных текстур
                        content.append(f"{tex_id} resources/textures/surfaces/bricks.png")
            content.append("")
        
        # Полы
        if self.used_textures['floor']:
            content.append("[FLOOR_TEXTURES]")
            content.append("# Формат: ID путь_к_текстуре")
            for tex_id in sorted(self.used_textures['floor']):
                if tex_id in self.textures:
                    name, color, photo, path = self.textures[tex_id]
                    if path:
                        try:
                            rel_path = path.relative_to(self.project_root)
                            content.append(f"{tex_id} {rel_path}")
                        except:
                            content.append(f"{tex_id} {path}")
                    else:
                        content.append(f"{tex_id} resources/textures/surfaces/parquet.png")
            content.append("")
        
        # Потолки
        if self.used_textures['ceiling']:
            content.append("[CEILING_TEXTURES]")
            content.append("# Формат: ID путь_к_текстуре")
            for tex_id in sorted(self.used_textures['ceiling']):
                if tex_id in self.textures:
                    name, color, photo, path = self.textures[tex_id]
                    if path:
                        try:
                            rel_path = path.relative_to(self.project_root)
                            content.append(f"{tex_id} {rel_path}")
                        except:
                            content.append(f"{tex_id} {path}")
                    else:
                        content.append(f"{tex_id} resources/textures/surfaces/boards.png")
            content.append("")
        
        # Мебель
        content.append("[FURNITURE_TYPES]")
        content.append("# Загружаем типы мебели")
        # Используем набор использованной мебели
        for fur_type in sorted(self.used_furniture):
            if fur_type in self.furniture_types:
                name, color, photo, path = self.furniture_types[fur_type]
                if path:
                    try:
                        rel_path = path.relative_to(self.project_root)
                        content.append(f"{fur_type} {rel_path}")
                    except:
                        content.append(f"{fur_type} {path}")
        
        # Добавляем все доступные типы мебели, если нет использованных
        if not self.used_furniture and self.furniture_types:
            for fur_type in self.furniture_types:
                name, color, photo, path = self.furniture_types[fur_type]
                if path:
                    try:
                        rel_path = path.relative_to(self.project_root)
                        content.append(f"{fur_type} {rel_path}")
                    except:
                        content.append(f"{fur_type} {path}")
        
        content.append("")
        
        # Добавляем объекты мебели
        if self.current_level['furniture']:
            content.append("[FURNITURE_OBJECTS]")
            content.append("# Добавляем объекты на уровень")
            content.append("# Формат: type_name x y [rotation] [scale]")
            for furniture in self.current_level['furniture']:
                content.append(f"{furniture['type']} {furniture['x']} {furniture['y']} {furniture['rotation']} {furniture['scale']}")
            content.append("")
        
        # Добавляем слои
        content.append("[WALL_LAYER]")
        for row in self.current_level['wall']:
            content.append(" ".join(str(cell) for cell in row[:width]))
        
        content.append("")
        content.append("[FLOOR_LAYER]")
        for row in self.current_level['floor']:
            content.append(" ".join(str(cell) for cell in row[:width]))
        
        content.append("")
        content.append("[CEILING_LAYER]")
        for row in self.current_level['ceiling']:
            content.append(" ".join(str(cell) for cell in row[:width]))
        
        return "\n".join(content)

def main():
    root = tk.Tk()
    app = Cartographer(root)
    root.mainloop()

if __name__ == "__main__":
    main()
