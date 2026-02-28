import React, { useState, useEffect, useRef } from 'react';
import { Send, BotMessageSquare, Bot, User, RefreshCcw } from 'lucide-react';
import { motion, AnimatePresence } from 'framer-motion';

function App() {
  const [input, setInput] = useState('');
  const [messages, setMessages] = useState([
    { text: "System online. Hybrid C++ Backbone active.", isBotMessageSquare: true }
  ]);
  const messagesEndRef = useRef(null);

  // Auto-scroll to the latest message
  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
  };

  useEffect(() => {
    scrollToBottom();
  }, [messages]);

  const handleSend = async () => {
    if (!input.trim()) return;

    const userMsg = { text: input, isBotMessageSquare: false };
    setMessages((prev) => [...prev, userMsg]);
    setInput('');

    try {
      // Talking to the C++ Crow Server via the Vite Proxy
      const response = await fetch('/chat', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ message: input }),
      });
      const data = await response.json();
      
      setMessages((prev) => [...prev, { text: data.reply, isBotMessageSquare: true }]);
    } catch (error) {
      setMessages((prev) => [...prev, { text: "Error: Is the C++ server running on port 18080?", isBotMessageSquare: true }]);
    }
  };

  const handleReload = async () => {
    try {
      await fetch('/reload');
      alert("C++ Rules Reloaded!");
    } catch (error) {
      alert("Reload failed. Check C++ terminal.");
    }
  };

  return (
    <div className="flex flex-col h-screen bg-slate-900 text-slate-100">
      {/* Header */}
      <header className="p-4 bg-slate-800 border-b border-slate-700 flex justify-between items-center">
        <div className="flex items-center gap-3">
          <div className="bg-blue-600 p-2 rounded-xl">
            <Bot size={24} className="text-white" />
          </div>
          <h1 className="font-bold text-xl tracking-tight">Hybrid Chatty</h1>
        </div>
        <button onClick={handleReload} className="p-2 hover:bg-slate-700 rounded-full transition-colors text-slate-400">
          <RefreshCcw size={20} />
        </button>
      </header>

      {/* Chat History */}
      <main className="flex-1 overflow-y-auto p-6 space-y-6">
        <AnimatePresence>
          {messages.map((msg, idx) => (
            <motion.div
              initial={{ opacity: 0, y: 10 }}
              animate={{ opacity: 1, y: 0 }}
              key={idx}
              className={`flex ${msg.isBotMessageSquare ? 'justify-start' : 'justify-end'}`}
            >
              <div className={`flex gap-3 max-w-[80%] ${msg.isBotMessageSquare ? 'flex-row' : 'flex-row-reverse'}`}>
                <div className={`p-2 rounded-full h-8 w-8 flex items-center justify-center ${msg.isBotMessageSquare ? 'bg-slate-700 text-blue-400' : 'bg-blue-600'}`}>
                  {msg.isBotMessageSquare ? <BotMessageSquare size={16} /> : <User size={16} />}
                </div>
                <div className={`p-4 rounded-2xl shadow-lg ${msg.isBotMessageSquare ? 'bg-slate-800 text-slate-200 rounded-tl-none border border-slate-700' : 'bg-blue-600 text-white rounded-tr-none'}`}>
                  {msg.text}
                </div>
              </div>
            </motion.div>
          ))}
        </AnimatePresence>
        <div ref={messagesEndRef} />
      </main>

      {/* Input Field */}
      <footer className="p-6 bg-slate-800 border-t border-slate-700">
        <div className="max-w-4xl mx-auto flex gap-4">
          <input
            type="text"
            value={input}
            onChange={(e) => setInput(e.target.value)}
            onKeyDown={(e) => e.key === 'Enter' && handleSend()}
            placeholder="Type a message..."
            className="flex-1 bg-slate-900 border border-slate-700 rounded-xl px-4 py-3 focus:outline-none focus:ring-2 focus:ring-blue-500"
          />
          <button onClick={handleSend} className="bg-blue-600 text-white p-3 rounded-xl hover:bg-blue-500 transition-all shadow-lg shadow-blue-900/20">
            <Send size={20} />
          </button>
        </div>
      </footer>
    </div>
  );
}

export default App;