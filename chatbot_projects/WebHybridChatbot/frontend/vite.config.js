import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import tailwindcss from '@tailwindcss/vite' 

export default defineConfig({
  plugins: [
    react(),
    tailwindcss(), 
  ],
  server: {
    proxy: {
      // This tells React to send chatbot requests to your C++ server
      '/chat': 'http://localhost:18080',
      '/reload': 'http://localhost:18080'
    }
  }
})