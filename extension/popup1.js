const input = document.getElementById('input');
const canvas = document.getElementById('heatmap');
const ctx = canvas.getContext('2d');

const crcTable = (() => {
    let table = [];
    for (let n = 0; n < 256; n++) {
        let c = n;
        for (let k = 0; k < 8; k++) c = ((c & 1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1));
        table[n] = c;
    }
    return table;
})();

function verifyCRC(matrix, expected) {
    let crc = -1;
    matrix.forEach(row => {
        for (let i = 0; i < row.length; i++) crc = (crc >>> 8) ^ crcTable[(crc ^ row.charCodeAt(i)) & 0xFF];
    });
    return (crc ^ -1) >>> 0 === expected;
}

function render(matrix) {
    const size = matrix.length;
    if (!size) return;
    const cell = canvas.width / size;
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    matrix.forEach((row, y) => {
        for (let x = 0; x < row.length; x++) {
            ctx.fillStyle = row[x] === '1' ? '#00d4ff' : '#111';
            ctx.fillRect(x * cell, y * cell, cell - 1, cell - 1);
        }
    });
}

input.addEventListener('input', () => {
    chrome.runtime.sendNativeMessage('com.bracket.audit', {text: input.value}, (res) => {
        if (res && res.matrix) {
            document.getElementById('status').innerText = verifyCRC(res.matrix, res.checksum) ? "Verified ✅" : "CRC Error ❌";
            render(res.matrix);
        }
    });
});

document.getElementById('export').onclick = () => {
    const link = document.createElement('a');
    link.download = 'audit_heatmap.png';
    link.href = canvas.toDataURL();
    link.click();
};
