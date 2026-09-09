let counter = 0;
document.getElementById('btn').addEventListener('click', () => {
    alert('Hello from the external script!');
    counter++;
    document.getElementById('counter').innerText = `Button clicked ${counter} times.`;
});
