
from bokeh.charts import save, output_file, BoxPlot
from bokeh.layouts import column, gridplot
from bokeh.palettes import all_palettes
from bokeh.plotting import figure
from bokeh.models.widgets import Panel, Tabs, Div
from bokeh.models.widgets import DataTable, TableColumn
from bokeh.models import ColumnDataSource

import pandas as pd
import numpy as np

import itertools


def create_colors(count):
    c = min(20, max(3, count))
    return list(itertools.islice(
        itertools.cycle(all_palettes['Category20'][c]), 0, count))


def create_timelines(report):
    worker_timelines, group_names = report.get_timelines()
    worker_timelines = list(worker_timelines.items())
    worker_timelines.sort()

    y_labels = []
    for worker_id, timelines in worker_timelines:
        for i, _ in enumerate(timelines):
            if worker_id == -1:
                y_labels.append("Server")
            else:
                y_labels.append("Worker {}.{}".format(worker_id, i))

    f = figure(plot_width=1000, plot_height=1000, y_range=y_labels,
               x_range=[0, report.end_time],
               webgl=True)

    line_id = 1

    colors = create_colors(len(group_names))
    for worker_id, timelines in worker_timelines:
        for starts, ends, gids in timelines:
            y = line_id
            c = [colors[i] for i in gids]
            f.quad(starts, ends, y - 0.1, y + 0.1, line_width=0,
                   fill_color=c, line_color=None)
            line_id += 1

    for i, group_name in enumerate(group_names):
            f.quad([], [], 1, 1,
                   fill_color=colors[i],
                   line_color="black",
                   legend=group_name)
    return f


def create_monitoring(report):
    result = []
    for worker in report.worker_list:
        f = figure(plot_width=1000, plot_height=130,
                   x_range=[0, report.end_time],
                   title="Worker {}".format(worker.address))
        f.line(worker.monitoring.time, worker.monitoring.cpu,
               color="blue", legend="CPU %")
        f.line(worker.monitoring.time, worker.monitoring.mem,
               color="red", legend="mem %")
        result.append([f])
    return gridplot(result)


def create_transfer(report):
    result = []
    for worker in report.worker_list:
        f = figure(plot_width=1000, plot_height=130,
                   x_range=[0, report.end_time],
                   title="Worker {}".format(worker.address))
        f.line(worker.sends.time, worker.sends.data_size.cumsum(),
               color="green", legend="send")
        f.line(worker.recvs.time, worker.recvs.data_size.cumsum(),
               color="red", legend="receive")
        result.append([f])
    return gridplot(result)


def create_ctransfer(report):
    sends = report.all_sends()
    f = figure(plot_width=1000, plot_height=500,
               x_range=[0, report.end_time],
               title="Cumulative transfers")
    sends = sends.join(report.task_frame["group"], on="id")
    names = report.group_names
    f = figure(plot_width=1000, plot_height=500, x_range=[0, report.end_time])
    for color, name in zip(create_colors(len(names)), names):
        frame = sends[sends["group"] == name]
        frame = frame.sort_values("time")
        f.line(frame.time, frame.data_size.cumsum(),
               color=color, legend=name, line_width=3)
    return f


def create_ctime(report):
    ds = report.task_frame
    names = ds["group"].unique()
    names.sort()
    colors = create_colors(len(names))

    f1 = figure(plot_width=1000, plot_height=400,
                x_range=[0, report.end_time],
                title="Cumulative time of finished tasks")
    for name, color in zip(names, colors):
        frame = ds[ds["group"] == name]
        f1.line(frame.end_time, frame.duration.cumsum(),
                legend=name, color=color, line_width=2)

    f2 = figure(plot_width=1000, plot_height=400,
                x_range=[0, report.end_time],
                title="Number of finished tasks")

    for name, color in zip(names, colors):
        frame = ds[ds["group"] == name]
        f2.line(frame.end_time, np.arange(1, len(frame) + 1),
                legend=name, color=color, line_width=2)
    return column([f1, f2])


def create_task_summary(report):
    counts = report.task_frame.group.value_counts().sort_index()

    groups = counts.index
    counts.reset_index(drop=True)

    ds = pd.DataFrame({"group": groups, "count": counts})
    ds.reset_index(inplace=True)

    source = ColumnDataSource(ds)

    columns = [
        TableColumn(field="group", title="Task"),
        TableColumn(field="count", title="Count"),
    ]
    table = DataTable(source=source, columns=columns, width=400, height=280)
    return table


def create_pending_tasks(report):
    ds = report.get_pending_tasks()
    f1 = figure(plot_width=1000, plot_height=400,
                x_range=[0, report.end_time])
    names = report.group_names
    for color, name in zip(create_colors(len(names)), names):
        frame = ds[ds["group"] == name]
        f1.line(frame.time, frame.change.cumsum(),
                color=color, legend=name, line_width=2)

    f2 = figure(plot_width=1000, plot_height=400)
    f2.line(ds.time, ds.change.cumsum(), line_width=2)
    return column([f2, f1])


def create_scheduling_time(report):
    ds = report.scheduler_times
    duration = ds["end_time"] - ds["start_time"]

    df = pd.DataFrame({"time": ds["end_time"], "duration": duration})
    df["label"] = 0
    f1 = figure(plot_width=1000, plot_height=400,
                x_range=[0, report.end_time])
    f1.line(df["time"], df["duration"].cumsum())
    f2 = BoxPlot(df, values="duration", label="label")
    return column([f1, f2])


def create_worker_load(report):
    data = []
    for task in report.task_frame.itertuples():
        data.append((task.start_time, 1, task.worker))
        data.append((task.end_time, -1, task.worker))
    frame = pd.DataFrame(data, columns=["time", "change", "worker"])
    frame.sort_values("time", inplace=True)
    result = []
    for worker in report.worker_list:
        f = figure(plot_width=1000, plot_height=130,
                   x_range=[0, report.end_time],
                   title="Worker {}".format(worker.address))
        data = frame[frame.worker == worker.worker_id]
        f.line(data.time, data.change.cumsum(),
               color="blue", legend="# tasks")
        result.append([f])
    return gridplot(result)


def create_task_durations(report):
    ds = report.task_frame
    f = BoxPlot(ds, values="duration", label="group", color="group")
    return f


def level_index(level):
    return ("brief", "normal", "full").index(level)


def create_html(report, filename, full):
    output_file(filename)

    structure = (
        ("Tasks",
         [("Task progress", create_ctime, False),
          ("Task summary", create_task_summary, False),
          ("Task durations", create_task_durations, False)]),
        ("Monitoring",
         [("CPU & Memory usage", create_monitoring, False)]),
        ("Scheduling",
         [("Timeline", create_timelines, True),
          ("Pending tasks", create_pending_tasks, False),
          ("Scheduling time", create_scheduling_time, False),
          ("Worker load", create_worker_load, False)]),
        ("Communication",
         [("Transfer per tasks", create_ctransfer, False),
          ("Transfer per nodes", create_transfer, False)])
    )

    tabs = []
    for name, subtabs in structure:
        print("Tab:", name)
        tabs2 = []
        for name2, fn, full_only in subtabs:
            if full_only and not full:
                print("    - {} ... SKIPPED".format(name2))
                f = Div(text="""Chart is disabled.
                                Use parameter <strong>--full</strong>
                                to enabled this graph.""")
            else:
                print("    - {} ...".format(name2))
                f = fn(report)
            tabs2.append(Panel(child=f, title=name2))
        tabs.append(Panel(child=Tabs(tabs=tabs2), title=name))

    print("Saving results ...")
    main = Tabs(tabs=tabs)
    save(main)
