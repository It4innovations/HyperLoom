
from bokeh.charts import save, output_file, BoxPlot
from bokeh.layouts import column, gridplot
from bokeh.palettes import all_palettes
from bokeh.plotting import figure
from bokeh.models.widgets import Panel, Tabs
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
    print("\tTimeline")

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

    f = figure(plot_width=1000, plot_height=1000, y_range=y_labels, webgl=True)

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
               title="Cumulative transfers")
    sends = sends.join(report.task_frame["group"], on="id")
    names = report.group_names
    f = figure(plot_width=1000, plot_height=500)
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
                title="Cumulative time of finished tasks")
    for name, color in zip(names, colors):
        frame = ds[ds["group"] == name]
        f1.line(frame.end_time, frame.duration.cumsum(),
                legend=name, color=color, line_width=2)

    f2 = figure(plot_width=1000, plot_height=400,
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
    print("\tPending tasks")
    ds = report.get_pending_tasks()
    f1 = figure(plot_width=1000, plot_height=400)
    names = report.group_names
    for color, name in zip(create_colors(len(names)), names):
        frame = ds[ds["group"] == name]
        f1.line(frame.time, frame.change.cumsum(),
                color=color, legend=name, line_width=2)

    f2 = figure(plot_width=1000, plot_height=400)
    f2.line(ds.time, ds.change.cumsum(), line_width=2)
    return column([f2, f1])


def create_scheduling_time(report):
    print("\tScheduling time")
    ds = report.scheduler_times
    duration = ds["end_time"] - ds["start_time"]

    df = pd.DataFrame({"time": ds["end_time"], "duration": duration})
    df["label"] = 0
    f1 = figure(plot_width=1000, plot_height=400)
    f1.line(df["time"], df["duration"].cumsum())
    f2 = BoxPlot(df, values="duration", label="label")
    return column([f1, f2])


def create_worker_load(report):
    print("\tWorker load")
    data = []
    for task in report.task_frame.itertuples():
        data.append((task.start_time, 1, task.worker))
        data.append((task.end_time, -1, task.worker))
    frame = pd.DataFrame(data, columns=["time", "change", "worker"])
    frame.sort_values("time", inplace=True)
    result = []
    for worker in report.worker_list:
        f = figure(plot_width=1000, plot_height=130,
                   title="Worker {}".format(worker.address))
        data = frame[frame.worker == worker.worker_id]
        f.line(data.time, data.change.cumsum(),
               color="blue", legend="# tasks")
        result.append([f])
    return gridplot(result)


def create_html(report, filename):
    output_file(filename)

    print("Task plots")
    tasks = Tabs(tabs=[
        Panel(child=create_ctime(report), title="Task progress"),
        Panel(child=create_task_summary(report), title="Task summary"),
    ])

    print("Monitoring plots")
    monitoring = Tabs(tabs=[
        Panel(child=create_monitoring(report), title="CPU & Memory usage"),
    ])

    print("Scheduling plots")
    scheduling = Tabs(tabs=[
        Panel(child=create_timelines(report), title="Timeline"),
        Panel(child=create_pending_tasks(report), title="Pending tasks"),
        Panel(child=create_scheduling_time(report), title="Scheduling time"),
        Panel(child=create_worker_load(report), title="Worker load"),
    ])

    print("Communication plots")
    comm = Tabs(tabs=[
        Panel(child=create_ctransfer(report), title="Transfer per tasks"),
        Panel(child=create_transfer(report), title="Transfer per nodes"),
    ])

    main = Tabs(tabs=[
        Panel(child=tasks, title="Tasks"),
        Panel(child=monitoring, title="Monitoring"),
        Panel(child=scheduling, title="Scheduling"),
        Panel(child=comm, title="Communication"),
    ])

    print("Saving result")
    save(main)
