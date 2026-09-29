// Compatibility shims for old QtWebEngine (Chromium) used by Qt 5.
// Mermaid v10.9.1 uses structuredClone (Chrome 98+) at load time and
// Object.hasOwn (Chrome 93+) while rendering flow charts, while Qt 5.15.10
// ships Chromium 87 (5.15.0-5.15.2 ships Chromium 83, which also lacks
// String.prototype.replaceAll (Chrome 85)). Without these shims,
// mermaid.min.js throws during loading, window.mermaid stays undefined and
// no Mermaid diagram can be rendered at all.
if (typeof structuredClone === 'undefined') {
    window.structuredClone = function(p_value) {
        return p_value === undefined ? undefined : JSON.parse(JSON.stringify(p_value));
    };
}

if (!Object.hasOwn) {
    Object.hasOwn = function(p_obj, p_key) {
        return Object.prototype.hasOwnProperty.call(p_obj, p_key);
    };
}

if (!String.prototype.replaceAll) {
    Object.defineProperty(String.prototype, 'replaceAll', {
        value: function(p_search, p_replacement) {
            if (p_search instanceof RegExp) {
                if (!p_search.global) {
                    throw new TypeError('replaceAll must be called with a global RegExp');
                }
                return this.replace(p_search, p_replacement);
            }
            return this.split(p_search).join(p_replacement);
        },
        writable: true,
        configurable: true
    });
}

class Mermaid extends GraphRenderer {
    constructor() {
        super();

        this.name = 'mermaid';

        this.graphDivClass = 'vx-mermaid-graph';

        this.extraScripts = [this.scriptFolderPath + '/mermaid/mermaid.min.js'];

        // default/dark/forest/neutral.
        this.theme = 'default';

        this.langs = ['mermaid'];
    }

    initialize(p_callback) {
        return super.initialize(() => {
           mermaid.initialize({
               startOnLoad: false,
               theme: this.theme
           });
            p_callback();
        });
    }

    // Render @p_node as Mermaid graph.
    // Return true on success.
    async renderOne(p_node, p_idx) {
        let graphSvg = null;
        try {
            const { svg } = await mermaid.render('vx-mermaid-graph-' + p_idx,
                                                 p_node.textContent);
            graphSvg = svg;
        } catch (p_err) {
            console.error('failed to render Mermaid', p_err);
            // Clean the container element, or Mermaid won't render the graph with
            // the same id.
            let graphNode = document.getElementById('vx-mermaid-graph-' + p_idx);
            if (graphNode) {
                let parentNode = graphNode.parentElement;
                parentNode.outerHTML = '';
                delete graphNode.parentElement;
            }
            this.finishRenderingOne();
            return false;
        }

        if (!graphSvg) {
            this.finishRenderingOne();
            return false;
        }

        let graphDiv = document.createElement('div');
        graphDiv.classList.add(this.graphDivClass);
        try {
            graphDiv.innerHTML = graphSvg;
            window.vxImageViewer.setupSVGToView(graphDiv.children[0], true);
        } catch (p_err) {
            console.error('incorrect graph SVG definition', p_err);
            this.finishRenderingOne();
            return false;
        }

        Utils.checkSourceLine(p_node, graphDiv);

        Utils.replaceNodeWithPreCheck(p_node, graphDiv);

        this.finishRenderingOne();
        return true;
    }

    // Render a graph from @p_text.
    // Will append a div to @p_container and return the div.
    async renderTextInternal(p_container, p_text, p_idx) {
        let graphSvg = null;
        try {
            const { svg } = await mermaid.render('vx-mermaid-graph-stand-alone-' + p_idx,
                                                 p_text);
            graphSvg = svg;
        } catch (p_err) {
            console.error('failed to render Mermaid', p_err);
            // Clean the container element, or Mermaid won't render the graph with
            // the same id.
            let graphNode = document.getElementById('vx-mermaid-graph-stand-alone-' + p_idx);
            if (graphNode) {
                let parentNode = graphNode.parentElement;
                parentNode.outerHTML = '';
                delete graphNode.parentElement;
            }
            return null;
        }

        if (!graphSvg) {
            return null;
        }

        let graphDiv = document.createElement('div');
        try {
            graphDiv.innerHTML = graphSvg;
        } catch (p_err) {
            console.error('incorrect graph SVG definition', p_err);
            return null;
        }

        p_container.appendChild(graphDiv);
        console.log(graphDiv);
        return graphDiv;
    }

    // p_callback(graphDiv).
    async renderText(p_container, p_text, p_idx, p_callback) {
        if (!this.initialize(async () => {
                let graphDiv = await this.renderTextInternal(p_container, p_text, p_idx);
                p_callback(graphDiv);
            })) {
            return;
        }

        let graphDiv = await this.renderTextInternal(p_container, p_text, p_idx);
        console.log(graphDiv);
        p_callback(graphDiv);
    }
}

window.vxcore.registerWorker(new Mermaid());
