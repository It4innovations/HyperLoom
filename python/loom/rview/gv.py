class Arc(object):

    def __init__(self, node, data, color, style):
        self.node = node
        self.data = data
        self.color = color
        self.style = style


class Node(object):

    color = None
    fontcolor = None
    fillcolor = None
    label = ""
    html_label = None
    shape = None

    def __init__(self, key):
        self.key = key
        self.arcs = []

    def add_arc(self, node, data=None, color=None, style=None):
        self.arcs.append(Arc(node, data, color, style))

    def merge_arcs(self, merge_fn):
        if len(self.arcs) < 2:
            return
        node_to_arcs = {}
        for arc in self.arcs[:]:
            a = node_to_arcs.get(arc.node)
            if a is None:
                node_to_arcs[arc.node] = arc
            else:
                self.arcs.remove(arc)
                a.data = merge_fn(a.data, arc.data)

    def __repr__(self):
        return "<Node {}>".format(self.key)


class Graph(object):

    def __init__(self):
        self.nodes = {}

    @property
    def size(self):
        return len(self.nodes)

    def node_check(self, key):
        node = self.nodes.get(key)
        if node is not None:
            return (node, True)
        node = Node(key)
        self.nodes[key] = node
        return (node, False)

    def node(self, key):
        node = self.nodes.get(key)
        if node is not None:
            return node
        node = Node(key)
        self.nodes[key] = node
        return node

    def show(self):
        run_xdot(self.make_dot("G"))

    def write(self, filename):
        dot = self.make_dot("G")
        with open(filename, "w") as f:
            f.write(dot)

    def make_dot(self, name):
        stream = ["digraph " + name + " {\n"]
        stream.append("node [shape=box];")
        for node in self.nodes.values():
            extra = ""
            if node.fontcolor is not None:
                extra += " fontcolor=\"{}\"".format(node.fontcolor)
            if node.shape is not None:
                extra += " shape={}".format(node.shape)
            if node.color is not None:
                extra += " color=\"{}\"".format(node.color)
            if node.fillcolor is not None:
                extra += " style=filled fillcolor=\"{}\""\
                    .format(node.fillcolor)
            if node.html_label is not None:
                label = "<" + node.html_label + ">"
            else:
                label = "\"" + node.label + "\""
            stream.append("v{} [label={}{}]\n".format(
                id(node), label, extra))
            for arc in node.arcs:
                extra = ""
                if arc.data is not None:
                    extra = "label=\"{}\"".format(arc.data)
                if arc.color is not None:
                    extra += " color=\"{}\"".format(arc.color)
                if arc.style is not None:
                    extra += " style=\"{}\"".format(arc.style)
                stream.append("v{} -> v{} [{}]\n".format(
                    id(node), id(arc.node), extra))
        stream.append("}\n")
        return "".join(stream)

    def merge_arcs(self, merge_fn):
        for node in self.nodes.values():
            node.merge_arcs(merge_fn)


def run_xdot(dot):
    import subprocess
    import tempfile
    with tempfile.NamedTemporaryFile() as f:
        f.write(dot)
        f.flush()
        subprocess.call(("xdot", f.name))
